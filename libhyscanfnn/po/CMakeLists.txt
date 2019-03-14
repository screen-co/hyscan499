# set (DOMAIN ${HYSCAN_FNN_LIBRARY})
 
# set (LINGUAS ru)
 
# # Устанавливаем имя директории в которую будут устанавливаться уже готовые,
# # откомпилированные с помощью gettext файлы переводов *.mo., где:
# # locale - имя этой директории.
# set (LOCALE_INSTALL_DIR locale)
 
# # Задаем имя шаблона файла перевода *.pot.
# set (POT_FILE ${CMAKE_CURRENT_BINARY_DIR}/messages.pot)
 
# # Генерируем шаблон перевода, сканируя исходники приложения и
# # извлекая всё что подлежит переводу.
# # Еще помимо *.cpp файлов сюда можно подставлять и *.h файлы.
# execute_process (COMMAND intltool-extract 
#                  --type=gettext/glade
#                  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
# file (GLOB SOURCES ../*.c ../*.h)
# message(INFO "sddss   " ${CMAKE_CURRENT_SOURCE_DIR} "\n" " TRTRT " ${SOURCES} "\nasdas " ${DOMAIN})
# add_custom_target (translation ALL DEPENDS)
# add_custom_command (TARGET translation
#                     COMMAND xgettext --from-code UTF-8 -C -a -o ${POT_FILE} ${SOURCES})
 
# # Пробегаем по всем именам директорий (окализациям) и обрабатываем их gettext-ом.
# foreach (LANG ${LINGUAS})
#     file (GLOB _poFile ${LANG}/*.po)
#     message(INFO "File::: " ${_poFile})
#     if (EXISTS ${_poFile})
#         set (PO_FILE_NEW ${CMAKE_CURRENT_BINARY_DIR}/${LANG}.po)
#         set (GMO_FILE_NEW ${CMAKE_CURRENT_BINARY_DIR}/${LANG}.gmo)
#         add_custom_command (TARGET translation
#                             COMMAND msgmerge ${_poFile} ${POT_FILE} -o ${PO_FILE_NEW}
#                             COMMAND msgfmt -c -o ${GMO_FILE_NEW} ${PO_FILE_NEW})
#         install (FILES ${GMO_FILE_NEW} DESTINATION ${LOCALE_INSTALL_DIR}/${LANG}/LC_MESSAGES RENAME ${DOMAIN}.mo)
#     endif (EXISTS ${_poFile})
# endforeach (LANG)
