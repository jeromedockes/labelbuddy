TEMPLATE = aux

INSTALLER = installer

INPUT = $$PWD/config/config.xml $$PWD/packages
creator.input = INPUT
creator.output = $$INSTALLER
creator.commands = ../../bin/binarycreator -c $$PWD/config/config.xml -p $$PWD/packages ${QMAKE_FILE_OUT}
creator.CONFIG += target_predeps no_link combine

QMAKE_EXTRA_COMPILERS += creator
