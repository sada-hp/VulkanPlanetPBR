#include "Shaders/ShaderLibrary.hpp"

ShaderCodeDefinition(GShaders::MeshVS)
(
	layout(location = 0) in vec3 vertPosition;
	layout(location = 1) in uint vertSubmesh;
	layout(location = 2) in vec3 vertNormal;
	layout(location = 3) in vec3 vertTangent;
	layout(location = 4) in vec2 vertUV;
	layout(location = 5) in mat4 Matrix;

	layout(location = 0) out vec2 fragUV;
	layout(location = 1) out flat uint fragSubmesh;
	layout(location = 2) out vec3 fragNormal;
	layout(location = 3) out vec3 fragPosition;

	void main()
	{
		vec4 WorldPosition = Matrix * vec4(vertPosition, 1.0);
		// gl_Position = vec4(UBO.view_proj_no_offset * WorldPosition);
		gl_Position = vec4(UBO.view_proj * WorldPosition);

		mat3 NormalMatrix = mat3(Matrix);
		fragNormal = normalize(NormalMatrix * vertNormal);
		// fragPosition = WorldPosition.xyz + UBO.eye_pos.xyz;
		fragPosition = WorldPosition.xyz;
		fragSubmesh = vertSubmesh;
		fragUV = vertUV;
	}
);

ShaderCodeDefinition(GShaders::CompressedMeshVS)
(
	layout(location = 0) in uvec4 packedData;
	layout(location = 1) in mat4 Matrix;

	layout(location = 0) out vec2 fragUV;
	layout(location = 1) out flat uint fragSubmesh;
	layout(location = 2) out vec3 fragNormal;
	layout(location = 3) out vec3 fragPosition;

	vec3 unpackNormal(vec2 e) 
	{
		vec3 n = vec3(e.xy, 1.0 - abs(e.x) - abs(e.y));
		if (n.z < 0.0) 
		{
			n.xy = (1.0 - abs(n.yx)) * sign(n.xy);
		}
		return normalize(n);
	}

	void unpackInput(out vec3 p, out vec3 n, out vec2 uv, out uint submesh)
	{
		vec2 d0 = unpackHalf2x16(packedData.x);
		vec2 d1 = unpackHalf2x16(packedData.y);
		vec2 d2 = unpackHalf2x16(packedData.z);
		vec2 d3 = unpackHalf2x16(packedData.w);

		p = vec3(d0.x, d0.y, d1.x);
		n = unpackNormal(vec2(d1.y, d2.x));
		uv = vec2(d2.y, d3.x);
		submesh = uint(d3.y);
	}

	void main()
	{
		vec3 vertPosition; vec3 vertNormal; vec2 vertUV; uint vertSubmesh;
		unpackInput(vertPosition, vertNormal, vertUV, vertSubmesh);

		vec4 WorldPosition = Matrix * vec4(vertPosition, 1.0);
		gl_Position = vec4(UBO.view_proj * WorldPosition);

		mat3 NormalMatrix = mat3(Matrix);
		fragNormal = normalize(NormalMatrix * vertNormal);
		// fragPosition = WorldPosition.xyz + UBO.eye_pos.xyz;
		fragPosition = WorldPosition.xyz;
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
		Position = vec4(WorldPosition, Submesh);
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
		
		Position = vec4(WorldPosition, Submesh);
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