local folder = path.getbasename(_SCRIPT_DIR)

workspace(folder)
	configurations { "Debug", "Release" }
	location("build")
	platforms { "x64" }
	flags {"MultiProcessorCompile"}

project(folder)
	kind("ConsoleApp")
	language("C++")
	targetdir("bin/%{cfg.platform}_%{cfg.buildcfg}")
	objdir( "bin/obj/%{cfg.platform}_%{cfg.buildcfg}/%{prj.name}")
	debugdir( "bin/%{cfg.platform}_%{cfg.buildcfg}")
	files { "**.h", "**.cpp" }
   
	filter "configurations:Debug"
		defines { "DEBUG" }
		symbols "On"
      
	filter "configurations:Release"
		flags { "LinkTimeOptimization", "StaticRuntime" }
		defines { "NDEBUG" }
		optimize "On"
		runtime("Release")
	  
	configuration "gmake"
		buildoptions { "-fPIC" }
		buildoptions { "-Wall" }
		buildoptions { "-std=c++11" }
		buildoptions { "-Wno-unknown-pragmas" }
		linkoptions { "-static" }
		
if _ACTION == "clean" then
	os.rmdir("bin")
	os.rmdir("build")
end