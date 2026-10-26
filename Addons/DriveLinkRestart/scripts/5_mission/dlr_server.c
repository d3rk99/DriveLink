modded class MissionServer
{
    override protected void DL_SendTransition(string sid, DL_Film source)
    {
        // This is the accepted journey, not the stripped cosmetic departure preview.
        if (source)
        {
            string json, err;
            if (JsonFileLoader<DL_Film>.MakeData(source,json,err,false))
                DL_SendFilmRpc(sid,json,4810092);
        }
        super.DL_SendTransition(sid,source);
    }
}