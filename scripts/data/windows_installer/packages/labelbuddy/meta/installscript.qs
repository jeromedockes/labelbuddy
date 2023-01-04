function Component()
{
    // default constructor
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    if (systemInfo.productType === "windows") {
        component.addOperation("CreateShortcut", "@TargetDir@/labelbuddy.exe", "@StartMenuDir@/labelbuddy.lnk", "workingDirectory=@TargetDir@");
    }
}
