#version 330 core
layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;
out vec3 FragPos; //--- ��ü�� ��ġ���� �����׸�Ʈ ���̴��� ������.
out vec3 Normal; //--- ��ְ��� �����׸�Ʈ ���̴��� ������.
uniform mat4 modelMatrix; //--- �𵨸� ��ȯ��
uniform mat4 view; //--- ���� ��ȯ��
uniform mat4 projection; //--- ���� ��ȯ��
void main()
{
gl_Position = projection * view * modelMatrix * vec4(vPos, 1.0);

FragPos = vec3(modelMatrix * vec4(vPos, 1.0));
Normal = vNormal;
}