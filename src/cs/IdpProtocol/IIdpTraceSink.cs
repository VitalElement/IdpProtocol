namespace IdpProtocol
{
    public interface IIdpTraceSink
    {
        void Trace(IdpTracePacket packet);
    }
}
