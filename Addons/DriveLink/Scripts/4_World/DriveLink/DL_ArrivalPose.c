// Commit entity AND model hierarchy before using world-space seat transforms.
class DL_ArrivalPose
{
    static void Resume(Transport vehicle)
    {
        vehicle.DisableSimulation(false);dBodyActive(vehicle,ActiveState.ACTIVE);
        vehicle.Update();vehicle.Synchronize();
    }
    static void Commit(Transport vehicle, vector position, vector orientation)
    {
        vector matrix[4];Math3D.YawPitchRollMatrix(orientation,matrix);matrix[3]=position;
        vehicle.SetTransform(matrix);
        vehicle.Update();vehicle.Synchronize();
    }
    static float SeatError(Transport vehicle)
    {
        float error=0;
        vector root[4];vehicle.GetTransform(root);
        for(int i=0;i<vehicle.CrewSize();i++)
        {
            vector seat[4];vector seatLocal[4];vector expected[4];
            vehicle.CrewTransform(i,seatLocal);vehicle.CrewTransformWS(i,seat);
            Math3D.MatrixMultiply4(root,seatLocal,expected);
            error=Math.Max(error,vector.Distance(seat[3],expected[3]));
        }
        return error;
    }
}
