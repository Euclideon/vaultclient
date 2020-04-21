# Run this from the buildscripts folder
# Requires an environment variable called "ShaderConductor" that points to a folder containing the ShaderConductor executable
SC=${ShaderConductor}/ShaderConductorCmd.exe

SrcFolder="../src/gl/directx11/shaders"

GenerateShader () {
	SrcName=${1}
	DstFolder=${2}
	Target=${3}
	Version=${4}
	VertExt=${5}
	FragExt=${6}

	# Determine shader stage and extension
	if [[ ${SrcName} == *"Vertex"* ]]; then
		ShaderStage=vs
		ShaderExt=${VertExt}
	elif [[ ${SrcName} == *"Fragment"* ]]; then
		ShaderStage=ps
		ShaderExt=${FragExt}
	else
		echo "Unknown shader type!"
		exit 1
	fi

	DstName=$(echo ${SrcName} | sed "s%${SrcFolder}%${2}%" | sed "s%hlsl%${ShaderExt}%")

	"${SC}" -E main -I ${SrcName} -O ${DstName} -S ${ShaderStage} -T ${Target} -V "${Version}"
}

for SrcName in ${SrcFolder}/*.hlsl; do
	# Generate OpenGL shader
	GenerateShader ${SrcName} "../src/gl/opengl/shaders/desktop" glsl "330" vert frag

	# Generate OpenGL ES shader
	GenerateShader ${SrcName} "../src/gl/opengl/shaders/mobile" essl "300" vert frag

	# Generate Metal macOS shader
	GenerateShader ${SrcName} "../src/gl/metal/shaders/desktop" msl_macos "" metal metal

	# Generate Metal iOS shader
	GenerateShader ${SrcName} "../src/gl/metal/shaders/mobile" msl_ios "" metal metal
done

# Modify ESSL shaders
sed -i 's/out_var_/varying_/g' ../src/gl/opengl/shaders/mobile/*.vert
sed -i 's/in_var_/varying_/g' ../src/gl/opengl/shaders/mobile/*.frag

# Modify ESSL shaders to bypass bug in WebGL
sed -i 's/default:/case 0u:/g' ../src/gl/opengl/shaders/mobile/*
