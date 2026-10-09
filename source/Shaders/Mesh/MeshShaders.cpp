#include "Shaders/ShaderLibrary.hpp"

ShaderCodeDefinition(GShaders::MeshVS)
(
	layout(push_constant) uniform constants
	{
		layout(offset = 0) dmat4 Matrix;
	} Transform;

	layout(location = 0) in vec3 vertPosition;
	layout(location = 1) in uint vertSubmesh;
	layout(location = 2) in vec3 vertNormal;
	layout(location = 3) in vec3 vertTangent;
	layout(location = 4) in vec2 vertUV;

	layout(location = 0) out vec2 fragUV;
	layout(location = 1) out flat uint fragSubmesh;
	layout(location = 2) out vec3 fragNormal;
	layout(location = 3) out vec3 fragPosition;

	void main()
	{
		dvec4 WorldPositionFP64 = Transform.Matrix * dvec4(vertPosition, 1.0);
		gl_Position = vec4(UBO.view_proj * WorldPositionFP64);

		fragPosition = vec3(WorldPositionFP64);
		fragSubmesh = vertSubmesh;
		fragNormal = vertNormal;
		fragUV = vertUV;
	}
);

ShaderCodeDefinition(GShaders::MeshPS)
(
	layout(location = 0) out vec4 Color;

	layout(location = 0) in vec2 UV;
	layout(location = 1) in flat uint Submesh;
	layout(location = 2) in vec3 Normal;
	layout(location = 3) in vec3 WorldPosition;

	layout(set = 0, binding = 1) uniform sampler2D IrradianceLUT;
	layout(set = 0, binding = 2) uniform sampler3D InscatteringLUT;
	layout(set = 0, binding = 3) uniform sampler2D TransmittanceLUT;
	layout(set = 1, binding = 0) uniform sampler2D Albedo[];

	void main()
	{
		float Shade = mix(0.25, 1.0, max(dot(normalize(Normal.xyz), UBO.sun_dir.xyz), 0.0));
		Color = texture(Albedo[Submesh], UV);

		Color.rgb = pow(Color.rgb, vec3(2.2));
		Color.rgb *= Shade;

		SAtmosphere Atmosphere;
		GetAtmosphere(TransmittanceLUT, IrradianceLUT, InscatteringLUT, UBO.eye_pos.xyz, WorldPosition, UBO.sun_dir.xyz, Atmosphere);
		Color.rgb = Atmosphere.L * Color.rgb + Atmosphere.S;
	}
);

ShaderCodeDefinition(GShaders::MeshDefaultPS)
(
	layout(location = 0) out vec4 Color;

	layout(location = 0) in vec2 UV;
	layout(location = 1) in flat uint Submesh;
	layout(location = 2) in vec3 Normal;
	layout(location = 3) in vec3 WorldPosition;

	layout(set = 0, binding = 1) uniform sampler2D IrradianceLUT;
	layout(set = 0, binding = 2) uniform sampler3D InscatteringLUT;
	layout(set = 0, binding = 3) uniform sampler2D TransmittanceLUT;

	void main()
	{
		float Shade = mix(0.25, 1.0, max(dot(normalize(Normal.xyz), UBO.sun_dir.xyz), 0.0));

		vec2 Checker = fract(UV) * 2.0 - 1.0;
		Color.rb = mix(vec2(0.0), vec2(0.5), float(sign(Checker.x) == sign(Checker.y)));
		Color.ga = vec2(0.0, 1.0);

		Color.rgb = pow(Color.rgb, vec3(2.2));
		Color.rgb *= Shade;

		SAtmosphere Atmosphere;
		GetAtmosphere(TransmittanceLUT, IrradianceLUT, InscatteringLUT, UBO.eye_pos.xyz, WorldPosition, UBO.sun_dir.xyz, Atmosphere);
		Color.rgb = Atmosphere.L * Color.rgb + Atmosphere.S;
	}
);