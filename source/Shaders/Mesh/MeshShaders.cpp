#include "Shaders/ShaderLibrary.hpp"

ShaderCodeDefinition(GShaders::MeshVS)
(
	layout(push_constant) uniform constants
	{
		layout(offset = 0) mat4 Matrix;
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
		mat4 Matrix = Transform.Matrix;

		vec4 WorldPosition = Matrix * vec4(vertPosition, 1.0);
		gl_Position = vec4(UBO.view_proj_no_offset * WorldPosition);

		mat3 NormalMatrix = mat3(Matrix);
		fragNormal = normalize(NormalMatrix * vertNormal);
		fragPosition = WorldPosition.xyz + UBO.eye_pos.xyz;
		fragSubmesh = vertSubmesh;
		fragUV = vertUV;
	}
);

ShaderCodeDefinition(GShaders::MeshPS)
(
	layout(location = 0) out vec4 Color;
	layout(location = 1) out vec4 Normal;
	layout(location = 2) out vec4 Position;

	layout(location = 0) in vec2 UV;
	layout(location = 1) in flat uint Submesh;
	layout(location = 2) in vec3 VertNormal;
	layout(location = 3) in vec3 WorldPosition;

	layout(set = 1, binding = 0) uniform sampler2D Albedo[];

	void main()
	{
		Color = texture(Albedo[Submesh], UV);
		Position = vec4(WorldPosition, 1.0);
		Normal = vec4(VertNormal, 0.0);
	}
);

ShaderCodeDefinition(GShaders::MeshDefaultPS)
(
	layout(location = 0) out vec4 Color;
	layout(location = 1) out vec4 Normal;
	layout(location = 2) out vec4 Position;

	layout(location = 0) in vec2 UV;
	layout(location = 1) in flat uint Submesh;
	layout(location = 2) in vec3 VertNormal;
	layout(location = 3) in vec3 WorldPosition;

	void main()
	{
		vec2 Checker = fract(UV) * 2.0 - 1.0;
		Color.rb = mix(vec2(0.0), vec2(0.5), float(sign(Checker.x) == sign(Checker.y)));
		Color.ga = vec2(0.0, 1.0);
		
		Position = vec4(WorldPosition, 1.0);
		Normal = vec4(VertNormal, 0.0);
	}
);

ShaderCodeDefinition(GShaders::MeshCompositePS)
(
	layout(location = 0) out vec4 Color;
	layout(location = 0) in vec2 UV;

	layout(set = 0, binding = 1) uniform sampler2D IrradianceLUT;
	layout(set = 0, binding = 2) uniform sampler3D InscatteringLUT;
	layout(set = 0, binding = 3) uniform sampler2D TransmittanceLUT;
	layout(input_attachment_index = 0, set = 0, binding = 4) uniform subpassInput inputColor;
	layout(input_attachment_index = 1, set = 0, binding = 5) uniform subpassInput inputNormal;
	layout(input_attachment_index = 2, set = 0, binding = 6) uniform subpassInput inputPosition;

	void main()
	{
		Color = subpassLoad(inputColor);
		vec3 Position = subpassLoad(inputPosition).xyz;
		vec3 Normal = normalize(subpassLoad(inputNormal).xyz);

		float Shade = mix(0.1, 1.0, max(dot(Normal.xyz, UBO.sun_dir.xyz), 0.0));
		Color.rgb = Shade * pow(Color.rgb, vec3(1.0 / 2.2));

		SAtmosphere Atmosphere;
		GetAtmosphere(TransmittanceLUT, IrradianceLUT, InscatteringLUT, UBO.eye_pos.xyz, Position, UBO.sun_dir.xyz, Atmosphere);
		Color.rgb = Atmosphere.L * Color.rgb + Atmosphere.S;
	}
);