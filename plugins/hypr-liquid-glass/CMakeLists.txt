cmake_minimum_required(VERSION 3.25)
project(hypr-liquid-glass LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-fPIC -O3 -Wall -Wno-unused-variable -Wno-unused-parameter")

set(HYPRLAND_DEV_DIR "/nix/store/fp6pjv1hk3phr45s7z4d2y644hl3q2zz-hyprland-0.56.1+date=2026-07-27_5c9377c-dev")

add_library(hypr-liquid-glass SHARED
    main.cpp
)

target_include_directories(hypr-liquid-glass PRIVATE
    ${HYPRLAND_DEV_DIR}/include
    ${HYPRLAND_DEV_DIR}/include/hyprland
    ${HYPRLAND_DEV_DIR}/include/hyprland/src
    ${HYPRLAND_DEV_DIR}/include/hyprland/protocols
    /nix/store/zssgb9g9wmjmxpgqvsflfcs7bskdrzqp-hyprgraphics-0.5.1-dev/include
    /nix/store/nx5dis1am8dp45xfy1wbbb1bwi8cgjph-hyprutils-0.14.0-dev/include
    /nix/store/wfhzqc9y79jirx6ywrcp0bx5m1ffjhb6-hyprlang-0.6.8-dev/include
    /nix/store/pyka21jxkbbirdc8wk9gbmfp1azwkhn7-aquamarine-0.14.0-dev/include
    /nix/store/7f6vrf96zbcphimnc4br6if3cpxxdd4k-glslang-16.4.0-dev/include
    /nix/store/zx88hmmi1an1gfyrfbzm4ga77qii30d5-pixman-0.46.4/include
    /nix/store/zx88hmmi1an1gfyrfbzm4ga77qii30d5-pixman-0.46.4/include/pixman-1
    /nix/store/ypf25f10c6k7v81sh05sgqjnvdxvwbzj-cairo-1.18.4-dev/include
    /nix/store/ypf25f10c6k7v81sh05sgqjnvdxvwbzj-cairo-1.18.4-dev/include/cairo
    /nix/store/zxv1y8jrpisd6g076x9rr4vqkq2b855c-pango-1.57.1-dev/include
    /nix/store/zxv1y8jrpisd6g076x9rr4vqkq2b855c-pango-1.57.1-dev/include/pango-1.0
    /nix/store/yan7xnjlp3l7pg82qz46b7z7x9za4flf-wayland-1.23.1-dev/include
    /nix/store/xb0m5rc3gi06vkqgy7imdj10s1185cf6-libxkbcommon-1.13.2-dev/include
    /nix/store/z380njpj9d2hl07pbs0ih33yfk4j4i25-libinput-1.31.3-dev/include
    /nix/store/yyn2602fggin0dqgvicd4rhlj9391bl0-systemd-261-dev/include
    /nix/store/y33nbr1c0g0yipyp0q0bxlcdr72gm4ib-libdrm-2.4.134-dev/include
    /nix/store/y33nbr1c0g0yipyp0q0bxlcdr72gm4ib-libdrm-2.4.134-dev/include/libdrm
    /nix/store/yyb5qzc7r8myb3gdfh61wlp3bpav3rzb-mesa-gl-headers-25.1.0/include
    /nix/store/zxd9sjrf10s65hfxrb08027bajplg5ks-libglvnd-1.7.0-dev/include
    /nix/store/ihmfhq3v4l0sb9f3kflhprmkyf7x1md9-tomlplusplus-3.4.0/include
    /nix/store/q79bghii5sam7v9w24aqa00kvqmss0nk-glaze-7.9.0/include
)

add_definitions(-DGIT_COMMIT_HASH="5c9377c15f85c50648f35ca5a213754f95b93ca0")

set_target_properties(hypr-liquid-glass PROPERTIES
    PREFIX ""
    OUTPUT_NAME "hypr-liquid-glass"
)
