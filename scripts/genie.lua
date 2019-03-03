
solution "UtilsCollection"
	location				(path.join("../.project/", _ACTION))
	language				"C++"
	configurations			{ "Debug", "Release" }
	platforms				{ "x32", "x64" }
	objdir					("../.build/".._ACTION)

	defines					{ "_CRT_SECURE_NO_WARNINGS" }

	project "ResourceEmbedder"
		uuid				"fecff87e-9cc0-4134-a2b5-6ef0e73969b4"
		kind				"ConsoleApp"
		targetdir			"../.output/"

		files {
							"../ResourceEmbedder/**.cpp",
							"../ResourceEmbedder/**.h",

							"../externals/lz4/lib/lz4.c",
							"../externals/lz4/lib/lz4.h",
							"../externals/lz4/lib/lz4hc.c",
							"../externals/lz4/lib/lz4hc.h",

							"../externals/zstd/lib/zstd.h",
							"../externals/zstd/lib/common/**.c",
							"../externals/zstd/lib/common/**.h",
							"../externals/zstd/lib/compress/**.c",
							"../externals/zstd/lib/compress/**.h"
		}

		includedirs {
							"../externals/lz4/lib",
							"../externals/zstd/lib/",
							"../externals/zstd/lib/common",
							"../externals/zstd/lib/compress"
		}

		platforms{}

		configuration		"Debug"
			targetsuffix	"_d"
			platforms		"x64"
				targetsuffix	"_x64_d"
			platforms{}
			flags			{ "Symbols" }
			
		configuration		"Release"
			platforms		"x64"
				targetsuffix	"_x64"
			platforms{}
			flags			{ "Optimize" }


	project "U8toX"
		uuid				"83220d0c-8a77-4acb-af45-aedaad4df6b5"
		kind				"ConsoleApp"
		targetdir			"../.output/"

		files {
							"../u8tox/**.cpp",
							"../u8tox/**.h"
		}

		platforms{}

		configuration		"Debug"
			targetsuffix	"_d"
			platforms		"x64"
				targetsuffix	"_x64_d"
			flags			{ "Symbols" }
			
		configuration		"Release"
			platforms		"x64"
				targetsuffix	"_x64"
			platforms{}
			flags			{ "Optimize" }

	project "BooleanExpression"
		uuid				"16b264fc-24cc-487b-840d-24070a7d461b"
		kind				"ConsoleApp"
		targetdir			"../.output/"

		files {
							"../BooleanExpression/**.cpp",
							"../BooleanExpression/**.h"
		}

		platforms{}

		configuration		"Debug"
			targetsuffix	"_d"
			platforms		"x64"
				targetsuffix	"_x64_d"
			flags			{ "Symbols" }
			
		configuration		"Release"
			platforms		"x64"
				targetsuffix	"_x64"
			platforms{}
			flags			{ "Optimize" }

		
	project "StringUtils"
		uuid				"618ee57a-e754-46cf-9f9b-7923e531d970"
		kind				"StaticLib"
		targetdir			"../.output/"

		files {
							"../StringUtils/**.cpp",
							"../StringUtils/**.h"
		}

		platforms{}

		configuration		"Debug"
			targetsuffix	"_d"
			platforms		"x64"
				targetsuffix	"_x64_d"
			flags			{ "Symbols" }
			
		configuration		"Release"
			platforms		"x64"
				targetsuffix	"_x64"
			platforms{}
			flags			{ "Optimize" }

