modded class PlayerBase
{
    protected int m_DL_LastDeliveryEpoch;
    protected int m_DL_ReleasedDeliveryEpoch;
    protected int m_DL_AckDeliveryEpoch;
    protected int m_DL_DeliveryStable;
    protected bool m_DL_ObservingDelivery;
    protected bool m_DL_ServerOnlyDelivery;
    protected int m_DL_NextArrivalReport;
    void DL_ArrivalWaiting(string reason)
    {
        if(GetGame().GetTime()<m_DL_NextArrivalReport)return;
        m_DL_NextArrivalReport=GetGame().GetTime()+5000;
        DL_Log.Info("arrival client waiting: "+reason);
    }
    protected Transport m_DL_AuditVehicle;
    protected int m_DL_AuditSeat = -2;
    protected int m_DL_AuditUntil;

    void DL_TrackArrivalSeats(Transport vehicle)
    {
        m_DL_AuditVehicle=vehicle;m_DL_AuditUntil=GetGame().GetTime()+180000;m_DL_AuditSeat=-2;
    }
    override void CommandHandler(float pDt, int pCurrentCommandID, bool pCurrentCommandFinished)
    {
        super.CommandHandler(pDt,pCurrentCommandID,pCurrentCommandFinished);
        if(GetGame().IsClient() && GetGame().GetPlayer()==this && m_DL_AuditVehicle && GetGame().GetTime()<m_DL_AuditUntil)
        {
            int currentSeat=m_DL_AuditVehicle.CrewMemberIndex(this);
            if(currentSeat!=m_DL_AuditSeat)
            {
                m_DL_AuditSeat=currentSeat;
                DL_Log.Info("arrival seat change: seat "+currentSeat.ToString()+", vehicle "+m_DL_AuditVehicle.GetPosition().ToString()+", player "+GetPosition().ToString()+", hierarchy error "+DL_ArrivalPose.SeatError(m_DL_AuditVehicle).ToString());
            }
        }
    }

    // A local transform write alone is not proof that the next native physics/network frame accepted it.
    void DL_ObserveDelivery(Transport vehicle, vector position, vector orientation, int epoch, int attempt)
    {
        if(!vehicle || epoch!=m_DL_LastDeliveryEpoch || epoch<=m_DL_ReleasedDeliveryEpoch || m_DL_AckDeliveryEpoch==epoch){m_DL_ObservingDelivery=false;return;}
        HumanCommandVehicle command=GetCommand_Vehicle();
        if(!command || command.GetTransport()!=vehicle){m_DL_ObservingDelivery=false;return;}
        float error=vector.Distance(vehicle.GetPosition(),position);
        float seatError=DL_ArrivalPose.SeatError(vehicle);
        if(error>0.5 || seatError>0.5)
        {
            m_DL_DeliveryStable=0;
            if(!m_DL_ServerOnlyDelivery)DL_ArrivalPose.Commit(vehicle,position,orientation);
            else vehicle.Update();
        }
        else m_DL_DeliveryStable++;
        if(m_DL_DeliveryStable>=5)
        {
            m_DL_AckDeliveryEpoch=epoch;m_DL_ObservingDelivery=false;
            GetGame().RPCSingleParam(this,DL_Const.RPC_STAGE_ACK,new Param2<int,vector>(epoch,vehicle.GetPosition()),true);
            DL_Log.Info("arrival client synchronized: epoch "+epoch.ToString()+", root error "+error.ToString()+", seat hierarchy error "+seatError.ToString());
            return;
        }
        if(attempt<30)GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(DL_ObserveDelivery,100,false,vehicle,position,orientation,epoch,attempt+1);
        else {m_DL_ObservingDelivery=false;DL_Log.Warn("arrival client still diverging; retaining protected assembly for retry");}
    }
    void DL_AuditDelivery(Transport vehicle, vector position)
    {
        if(vehicle)DL_Log.Info("arrival client audit: vehicle "+vehicle.GetPosition().ToString()+", player "+GetPosition().ToString()+", delivered "+position.ToString()+", seat hierarchy error "+DL_ArrivalPose.SeatError(vehicle).ToString());
        if(vehicle && dBodyIsSet(vehicle)){vector body[4];dBodyGetWorldTransform(vehicle,body);DL_Log.Info("arrival physics audit: "+body[3].ToString());}
        HumanCommandVehicle seated=GetCommand_Vehicle();
        if(vehicle && seated && seated.GetTransport()==vehicle)
        {
            int slot=vehicle.CrewMemberIndex(this);
            if(slot>=0)DL_Log.Info("arrival interaction audit: transitioning="+seated.IsTransitioning().ToString()+", throughDoor="+vehicle.CrewCanGetThrough(slot).ToString()+", doorAreaFree="+vehicle.IsAreaAtDoorFreeDiag(slot).ToString()+", disabled="+vehicle.GetIsSimulationDisabled().ToString());
        }
    }
    override void OnRPC(PlayerIdentity sender,int rpc_type,ParamsReadContext ctx)
    {
        super.OnRPC(sender,rpc_type,ctx);
        if(rpc_type==DL_Const.RPC_STAGE_CONTROL && GetGame().IsClient() && GetGame().GetPlayer()==this)
        {
            Transport vehicle;vector position,orientation;bool pause;int epoch;bool serverOnly;
            if(!ctx.Read(vehicle)||!ctx.Read(position)||!ctx.Read(orientation)||!ctx.Read(pause)||!ctx.Read(epoch)||!ctx.Read(serverOnly)){DL_ArrivalWaiting("delivery RPC unreadable");return;}
            m_DL_ServerOnlyDelivery=serverOnly;
            if(!vehicle){DL_ArrivalWaiting("destination vehicle has not streamed in");return;}
            if(epoch>0 && pause)
            {
                vehicle.Update();
                GetGame().RPCSingleParam(this,DL_Const.RPC_STAGE_VISIBLE,new Param1<int>(epoch),true);
            }
            HumanCommandVehicle command=GetCommand_Vehicle();
            if(!command || command.GetTransport()!=vehicle){DL_ArrivalWaiting("destination vehicle visible; waiting for seat command");return;}
            if(pause && epoch<=m_DL_ReleasedDeliveryEpoch && m_DL_ReleasedDeliveryEpoch>0)return;
            if(epoch>0 && epoch<m_DL_LastDeliveryEpoch)return;
            if(!pause)
            {
                m_DL_ReleasedDeliveryEpoch=epoch;
                if(!serverOnly)DL_ArrivalPose.Commit(vehicle,position,orientation);
                // Observation-only positioning must still release replicated simulation state.
                DL_ArrivalPose.Resume(vehicle);
                DL_Log.Info("arrival interaction release: disabled="+vehicle.GetIsSimulationDisabled().ToString()+", actionTarget="+vehicle.CanBeActionTarget().ToString()+", fuelCap="+DL_Veh.Capacity(vehicle,0).ToString()+", fuel="+DL_Veh.Fraction(vehicle,0).ToString());
                DL_TrackArrivalSeats(vehicle);
                GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(DL_AuditDelivery,3000,false,vehicle,position);
                return;
            }
            if(!serverOnly){vehicle.DisableSimulation(true);dBodyActive(vehicle,ActiveState.INACTIVE);SetVelocity(vehicle,vector.Zero);dBodySetAngularVelocity(vehicle,vector.Zero);}
            if(epoch<=0)return;
            if(epoch!=m_DL_LastDeliveryEpoch)
            {
                m_DL_LastDeliveryEpoch=epoch;m_DL_DeliveryStable=0;
                if(!serverOnly)DL_ArrivalPose.Commit(vehicle,position,orientation);
                else {DL_ArrivalPose.Resume(vehicle);DL_Log.Info("arrival server-only delivery: native replication; client observes without position writes");}
            }
            if(m_DL_AckDeliveryEpoch==epoch)GetGame().RPCSingleParam(this,DL_Const.RPC_STAGE_ACK,new Param2<int,vector>(epoch,vehicle.GetPosition()),true);
            else if(!m_DL_ObservingDelivery){m_DL_ObservingDelivery=true;GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(DL_ObserveDelivery,100,false,vehicle,position,orientation,epoch,0);}
        }
        if(rpc_type==DL_Const.RPC_STAGE_ACK && GetGame().IsServer() && sender && GetIdentity() && sender.GetPlainId()==GetIdentity().GetPlainId())
        {
            Param2<int,vector> ack;if(!ctx.Read(ack))return;
            HumanCommandVehicle seated=GetCommand_Vehicle();if(!seated || !seated.GetTransport())return;
            if(vector.Distance(ack.param2,seated.GetTransport().GetPosition())>1)return;
            Mission mission=GetGame().GetMission();if(mission && mission.MissionScript)mission.MissionScript.CallFunction(mission,"DL_StageAck",null,sender.GetPlainId()+"|"+ack.param1.ToString());
        }
        if(rpc_type==DL_Const.RPC_STAGE_VISIBLE && GetGame().IsServer() && sender && GetIdentity() && sender.GetPlainId()==GetIdentity().GetPlainId())
        {
            Param1<int> visible;if(!ctx.Read(visible))return;
            Mission waitingMission=GetGame().GetMission();
            if(waitingMission && waitingMission.MissionScript)waitingMission.MissionScript.CallFunction(waitingMission,"DL_StageVisible",null,sender.GetPlainId()+"|"+visible.param1.ToString());
        }
    }
}
