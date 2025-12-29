include("D:/qt6/codecodecode/QtTranslator/build/Desktop_Qt_6_9_2_MinGW_64_bit-Debug/.qt/QtDeploySupport.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/QtTranslator-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase")

qt6_deploy_runtime_dependencies(
    EXECUTABLE "D:/qt6/codecodecode/QtTranslator/build/Desktop_Qt_6_9_2_MinGW_64_bit-Debug/QtTranslator.exe"
    GENERATE_QT_CONF
)
