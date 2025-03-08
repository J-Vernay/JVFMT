workspace "JVFMT"

  platforms { "x64-windows" }
  configurations { "Debug", "Release" }
  
  location "build"
  targetdir "build/%{cfg.platform}/%{cfg.buildcfg}"
  symbols "On"
  cdialect "C11"
  
  warnings "Everything"
  defines { "_CRT_SECURE_NO_WARNINGS" }
  disablewarnings { "4820" } -- bytes padding added after data member
  disablewarnings { "5045" }
  
  filter "platforms:x64-windows"
    architecture "x86_64"
  filter {}

  filter "configurations:Release"
    optimize "On"
  filter {}

project "_none"
  kind "None"
  files { "../.clang-format", "./gitignore" }

project "_premake"
  kind "Utility"
  files { "premake5.lua" }
  prebuildcommands {
      "premake5 %{_ACTION} --file=%[premake5.lua]"
  }

project "JVFMT_Test"
  kind "ConsoleApp"
  files { "../jvfmt.h", "../jvfmt.c", "*.c", "*.h" }


