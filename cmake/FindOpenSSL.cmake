# - Try to find openssl include dirs and libraries
#
# Usage of this module as follows:
#
# find_package(OpenSSL)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
# OpenSSL_ROOT_DIR Set this variable to the root installation of
# openssl if the module has problems finding the
# proper installation path.
#
# Variables defined by this module:
#
# OPENSSL_FOUND System has openssl, include and library dirs found
# OpenSSL_INCLUDE_DIR The openssl include directories.
# OpenSSL_LIBRARIES The openssl libraries.
# OpenSSL_CYRPTO_LIBRARY The openssl crypto library.
# OpenSSL_SSL_LIBRARY The openssl ssl library.

find_path(OpenSSL_ROOT_DIR
    NAMES include/openssl/ssl.h
)

find_path(OpenSSL_INCLUDE_DIR
    NAMES openssl/ssl.h
    HINTS ${OpenSSL_ROOT_DIR}/include
)

find_library(OpenSSL_SSL_LIBRARY
    NAMES libeay32MT
    HINTS ${OpenSSL_ROOT_DIR}/lib
)
find_library(OpenSSL_LIB_LIBRARY
    NAMES ssleasy32MT
    HINTS ${OpenSSL_ROOT_DIR}/lib
)

set(OpenSSL_LIBRARIES ${OpenSSL_SSL_LIBRARY}
    CACHE STRING "OpenSSL SSL" FORCE)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenSSL DEFAULT_MSG
    OpenSSL_LIBRARIES
    OpenSSL_INCLUDE_DIR
)

mark_as_advanced(
    OpenSSL_ROOT_DIR
    OpenSSL_INCLUDE_DIR
    OpenSSL_LIBRARIES
    OpenSSL_SSL_LIBRARY
)