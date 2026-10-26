// One protection window for a complete party, including crews and convoys.
// Server-owned; a client cannot start or extend this lease.
class DL_ArrivalGuard
{
    int expected = 1;
    int timeoutAt;
    int expiresAt;
    ref map<string, int> members = new map<string, int>; // -1: assembling; otherwise story/release time
    ref map<Object, bool> originalDamage = new map<Object, bool>;

    bool Expired(int now, bool pending, int graceSeconds)
    {
        if(pending || (members.Count()<expected && now<timeoutAt)){expiresAt=0;return false;}
        if(expiresAt==0)expiresAt=now+Math.Max(60,graceSeconds)*1000;
        return now>=expiresAt;
    }
}
