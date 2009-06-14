dnl ---------------------------------------------------------------------------
dnl Macro: MYSQL_SRC
dnl ---------------------------------------------------------------------------
AC_DEFUN([MYSQL_SRC_TEST], [
  AC_MSG_CHECKING(for mysql source code)
  AC_ARG_WITH(mysql-src,
  [[  --with-mysql-src[=mysql src directory]
                        Source requir to build engine.]],
  [
    if test -x "$withval"; then
      MYSQL_SRC_INCLUDES="-I$withval/sql -I$withval/regex -I$withval/include"
    fi
  ],
  [
    AC_MSG_ERROR(["no mysql source provided. Please specify --with-mysql-src."])
  ])
])

dnl ---------------------------------------------------------------------------
dnl Macro: MYSQL_SRC
dnl ---------------------------------------------------------------------------

dnl ---------------------------------------------------------------------------
dnl Macro: MYSQL_CONFIG
dnl ---------------------------------------------------------------------------

AC_DEFUN([MYSQL_CONFIG_TEST], [
  AC_MSG_CHECKING(for mysql_config tool)
  AC_ARG_WITH(mysql-config,
  [[  --with-mysql-config[=mysql config path]
                        mysql config path require to build engine.]],
  [
    if test -x "$withval"; then
        MYSQL_INCLUDES=`$with_mysql_config --include`
        MYSQL_PLUGINDIR=`$with_mysql_config --plugindir`
    fi
  ],
  [
    AC_MSG_ERROR([mysql_config not found. Please specify --with-mysql-config.])
  ])
])


dnl ---------------------------------------------------------------------------
dnl Macro: MYSQL_CONFIG
dnl ---------------------------------------------------------------------------

