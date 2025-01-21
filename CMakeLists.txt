cmake_minimum_required(VERSION 3.26)
project(Temat_5 C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(Temat_5
        dyspozytor.c
        funkcje.h
        ksamolotu.c
        lotnisko.c
        mainp.c
        pasazer.c
        struktury.h)
