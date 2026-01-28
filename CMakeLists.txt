cmake_minimum_required(VERSION 3.20)
project(8086_Simulator C)

set(CMAKE_C_STANDARD 11)

# PostgreSQL yolları - Kendi bilgisayarındaki klasör adıyla (18) eşleştiğinden emin ol
set(PG_INCLUDE_DIR "C:/Program Files/PostgreSQL/18/include")
set(PG_LIB_DIR "C:/Program Files/PostgreSQL/18/lib")

# Klasörlerin varlığını kontrol et (Hata ayıklama için önemli)
if(NOT EXISTS "${PG_INCLUDE_DIR}")
    message(FATAL_ERROR "PostgreSQL include dizini bulunamadi: ${PG_INCLUDE_DIR}")
endif()

if(NOT EXISTS "${PG_LIB_DIR}")
    message(FATAL_ERROR "PostgreSQL lib dizini bulunamadi: ${PG_LIB_DIR}")
endif()

add_executable(8086_Simulator main.c)

# Derleyiciye .h dosyalarının yerini gösteriyoruz
target_include_directories(8086_Simulator PRIVATE "${PG_INCLUDE_DIR}")

# Derleyiciye .lib dosyalarının yerini gösteriyoruz
target_link_directories(8086_Simulator PRIVATE "${PG_LIB_DIR}")

# libpq kütüphanesini projeye bağlıyoruz
target_link_libraries(8086_Simulator PRIVATE libpq)

# Bilgi mesajları (Alt sekmede görünür)
message(STATUS "PostgreSQL Include Yolu: ${PG_INCLUDE_DIR}")
message(STATUS "PostgreSQL Lib Yolu: ${PG_LIB_DIR}")