R_DEFINE_NAMESPACE_RESULT_MODULE(ulaunch, 380);

namespace ulaunch {

    R_DEFINE_ERROR_RANGE(Misc, 1, 99);
    R_DEFINE_ERROR_RESULT(AssertionFailed, 1);
    R_DEFINE_ERROR_RESULT(InvalidTransform, 2);

    R_DEFINE_ERROR_RANGE(Smi, 101, 199);
    R_DEFINE_ERROR_RESULT(OutOfPushSpace, 101);
    R_DEFINE_ERROR_RESULT(OutOfPopSpace, 102);
    R_DEFINE_ERROR_RESULT(InvalidInHeaderMagic, 103);
    R_DEFINE_ERROR_RESULT(InvalidOutHeaderMagic, 104);
    R_DEFINE_ERROR_RESULT(WaitTimeout, 105);

    R_DEFINE_ERROR_RANGE(SystemSf, 201, 299);
    R_DEFINE_ERROR_RESULT(InvalidProcess, 201);
    R_DEFINE_ERROR_RESULT(NoMessagesAvailable, 202);

    R_DEFINE_ERROR_RANGE(Loader, 301, 399);
    R_DEFINE_ERROR_RESULT(InvalidProcessType, 301);
    R_DEFINE_ERROR_RESULT(InvalidTargetInputMagic, 302);
    R_DEFINE_ERROR_RESULT(InvalidTargetInputSize, 303);

    R_DEFINE_ERROR_RANGE(SystemSmi, 401, 499);
    R_DEFINE_ERROR_RESULT(ApplicationActive, 401);
    R_DEFINE_ERROR_RESULT(InvalidSelectedUser, 402);
    R_DEFINE_ERROR_RESULT(AlreadyQueued, 403);
    R_DEFINE_ERROR_RESULT(ApplicationNotActive, 404);
    R_DEFINE_ERROR_RESULT(NoHomebrewTakeoverApplication, 405);

    R_DEFINE_ERROR_RANGE(Util, 501, 599);
    R_DEFINE_ERROR_RESULT(InvalidJson, 501);

    R_DEFINE_ERROR_RANGE(Menu, 601, 699);
    R_DEFINE_ERROR_RESULT(RomfsNotFound, 601);

    R_DEFINE_ERROR_RANGE(Config, 701, 799);
    R_DEFINE_ERROR_RESULT(InvalidThemeZipFile, 701);
    R_DEFINE_ERROR_RESULT(ThemeManifestNotFound, 702);
    R_DEFINE_ERROR_RESULT(InvalidThemeZipFileRead, 703);
    R_DEFINE_ERROR_RESULT(ThemeManifestVersionNotFound, 704);
    R_DEFINE_ERROR_RESULT(ThemeManifestNameNotFound, 705);
    R_DEFINE_ERROR_RESULT(ThemeManifestAuthorNotFound, 706);
    R_DEFINE_ERROR_RESULT(ThemeManifestDescriptionNotFound, 707);
    R_DEFINE_ERROR_RESULT(ThemeManifestReleaseNotFound, 708);
    R_DEFINE_ERROR_RESULT(ThemeIconNotFound, 709);
    R_DEFINE_ERROR_RESULT(ThemeIconCacheFail, 710);

}
