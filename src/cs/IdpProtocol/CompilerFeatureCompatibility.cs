namespace System.Runtime.CompilerServices
{
#if NETSTANDARD2_0
    internal static class IsExternalInit
    {
    }

    [global::System.AttributeUsage(global::System.AttributeTargets.All, Inherited = false)]
    internal sealed class RequiredMemberAttribute : global::System.Attribute
    {
    }

    [global::System.AttributeUsage(global::System.AttributeTargets.All, Inherited = false)]
    internal sealed class CompilerFeatureRequiredAttribute : global::System.Attribute
    {
        public CompilerFeatureRequiredAttribute(string featureName)
        {
            FeatureName = featureName;
        }

        public string FeatureName { get; }

        public bool IsOptional { get; init; }
    }
#endif
}
