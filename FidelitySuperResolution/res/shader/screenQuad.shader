=type vertex

layout(location = 0) in vec2 aTexCoord;
out vec2 texCoord;

void main()
{
	texCoord = aTexCoord;
	texCoord.y = 1.0 - texCoord.y;
	vec2 transTex = aTexCoord * 2.0 - 1.0;
	gl_Position = vec4(transTex, 0.0, 1.0);
}

=type fragment

in vec2 texCoord;
out vec4 FragColor;

uniform sampler2D frameBuffer;

void main()
{
	vec3 color = texture(frameBuffer, texCoord).rgb;
	FragColor = vec4(color, 1.0);
}