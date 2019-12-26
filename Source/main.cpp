#include "../Externals/Include/Include.h"

using namespace glm;
using namespace std;

#define SHADOW_MAP_SIZE 4096

typedef struct _Shader
{
	GLuint program;
	GLuint texture;
	vector<GLuint> vaos;
	vector<int> index_counts;
} Shader;

typedef struct _Frame
{
	GLuint fbo;
	GLuint rbo;
	GLuint tex;
} Frame;

typedef struct _TextureData
{
	_TextureData(void) :
		width(0),
		height(0),
		data(0)
	{
	}

	int width;
	int height;
	unsigned char* data;
} TextureData;

GLfloat window_positions[] =
{
	1.0f,-1.0f,1.0f,0.0f,
	-1.0f,-1.0f,0.0f,0.0f,
	-1.0f,1.0f,0.0f,1.0f,
	1.0f,1.0f,1.0f,1.0f
};

struct
{
    struct
    {
        GLint inv_vp_matrix;
		GLint eye;
    } skybox;
    struct
    {
        GLint um4p;
		GLint um4m;
		GLint um4v;
		GLint um4shadow;
		GLint tex_cubemap;
		GLint tex_shadow;
    } model;
    struct
    {
        GLint um4mvp;
    } light;
    struct
    {
        GLint sobj_tex;
        GLint snoobj_tex;
        GLint sb_tex;
        GLint showmode;
    } window;
    struct
    {
        GLint um4mvp;
		GLint tex_shadow;
		GLint drawshadow;
		GLint shadow_matrix;
    } quad;
} uniforms;

// loading function
char** loadShaderSource(const char* file);
void loadModel();
TextureData loadImage(const char* const Filepath);

// opengl
void linkProgram(GLuint program, const char* vertexFile, const char* fragmentFile);
void bindFrameToTex(Frame &frame);

// glut
void My_Keyboard(unsigned char key, int x, int y);
void My_Mouse(int button, int state, int x, int y);
void My_MouseMove(int x,int y);
void My_MouseWheel(int button, int dir, int x, int y);

// trackball
vec3 projectToSphere(vec2 xy);
vec2 scaleMouse(vec2 mouse);

// draw
void drawModel();

// shader program
Shader skybox;
Shader model;
Shader quad;
Shader depth;
Shader window;
Frame depthmap;

// three framebuffer object for Differential rendering
Frame sobj, snoobj, sb;

// clock
unsigned int timer_speed = 20;

// camera
mat4 proj_matrix;
vec3 view_position;			 // a 3 dimension vector which represents how far did the ladybug should move
vec3 view_direction;
float rotate_angle=0.0f;
int width, height;
int showmode = 3;

// track ball
vec2 mouseXY;
float camera_speed=1;

void My_Init()
{
    glEnable(GL_DEPTH_TEST);

	// skybox
    glDepthFunc(GL_LEQUAL);

	skybox.program = glCreateProgram();
	linkProgram(skybox.program, "skybox.vs.glsl", "skybox.fs.glsl");
	glUseProgram(skybox.program);

	uniforms.skybox.inv_vp_matrix = glGetUniformLocation(skybox.program, "inv_vp_matrix");
	uniforms.skybox.eye = glGetUniformLocation(skybox.program, "eye");

	vector<string> faces
	{
		"cubemaps\\r.png",
		"cubemaps\\l.png",
		"cubemaps\\t.png",
		"cubemaps\\d.png",
		"cubemaps\\b.png",
		"cubemaps\\f.png"
	};

	glGenTextures(1, &skybox.texture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.texture);
	for(int i = 0; i < 6; ++i)
	{
		TextureData envmap_data = loadImage(faces[i].c_str());
		glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, envmap_data.width, envmap_data.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, envmap_data.data);
		delete[] envmap_data.data;
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    
	GLuint vao;
	skybox.vaos.push_back(vao);
    glGenVertexArrays(1, &skybox.vaos[0]);

	// model
	model.program = glCreateProgram();
	linkProgram(model.program, "model.vs.glsl", "model.fs.glsl");

	uniforms.model.um4p = glGetUniformLocation(model.program, "um4p");
    uniforms.model.um4m = glGetUniformLocation(model.program, "um4m");
    uniforms.model.um4v = glGetUniformLocation(model.program, "um4v");
    uniforms.model.um4shadow = glGetUniformLocation(model.program, "shadow_matrix");
    uniforms.model.tex_shadow = glGetUniformLocation(model.program, "tex_shadow");
    uniforms.model.tex_cubemap = glGetUniformLocation(model.program, "tex_cubemap");


	loadModel();

	// quad
	quad.program = glCreateProgram();
	linkProgram(quad.program, "quad.vs.glsl", "quad.fs.glsl");
	uniforms.quad.um4mvp = glGetUniformLocation(quad.program, "um4mvp");
	uniforms.quad.drawshadow = glGetUniformLocation(quad.program, "drawshadow");
	uniforms.quad.shadow_matrix = glGetUniformLocation(quad.program, "shadow_matrix");
	uniforms.quad.tex_shadow = glGetUniformLocation(quad.program, "tex_shadow");

	// depth
	depth.program = glCreateProgram();
	linkProgram(depth.program, "depth.vs.glsl", "depth.fs.glsl");
	uniforms.light.um4mvp = glGetUniformLocation(depth.program, "mvp");

	// window
	window.program = glCreateProgram();
	linkProgram(window.program, "differential.vs.glsl", "differential.fs.glsl");
	
	GLuint window_vao;
	window.vaos.push_back(window_vao);
	glGenVertexArrays(1, &window.vaos[0]);
	glBindVertexArray(window.vaos[0]);

	GLuint window_vbo;

	glGenBuffers(1, &window_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, window_vbo);
	glBufferData(GL_ARRAY_BUFFER,sizeof(window_positions), window_positions, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*4, 0);
	glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*4, (const GLvoid*)(sizeof(GL_FLOAT)*2));

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
    uniforms.window.sb_tex = glGetUniformLocation(window.program, "sb_tex");
    uniforms.window.snoobj_tex = glGetUniformLocation(window.program, "snoobj_tex");
	uniforms.window.sobj_tex = glGetUniformLocation(window.program, "sobj_tex");
	uniforms.window.showmode = glGetUniformLocation(window.program, "showmode");

	// shadow frameobject
	glGenFramebuffers(1, &depthmap.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, depthmap.fbo);

	glGenTextures(1, &depthmap.tex);
	glBindTexture(GL_TEXTURE_2D, depthmap.tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 0,  GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthmap.tex, 0);
	// other frameobject will be defined in my_Reshape
	glGenFramebuffers( 1, &sb.fbo );
	glGenFramebuffers( 1, &snoobj.fbo );
	glGenFramebuffers( 1, &sobj.fbo );


	// setup viewing position and rotation
	view_position = vec3(0.0f, 0.0f, 0.0f);
	view_direction = normalize(vec3(-1.0f, -1.0f, 0.0f));	
}

void My_Display()
{
	static const GLfloat white[] = { 1.0f, 1.0f, 1.0f, 1.0f };

	//===== model transform
	mat4 translation_matrix = translate(mat4(), vec3(-10, -13, -8));
	mat4 scale_matrix = scale(mat4(), vec3(0.5f, 0.35f, 0.5f));
	mat4 model_matrix = translation_matrix * scale_matrix;
	model_matrix = rotate(model_matrix, radians(rotate_angle), vec3(0.0, 1.0, 0.0));
	
	translation_matrix = translate(mat4(), vec3(-13, -13, 0));
	scale_matrix = scale(mat4(), vec3(13, 13, 13));
	mat4 quad_matrix = translation_matrix * scale_matrix;

	// ====== draw shadow map pass
	const float shadow_range = 20.0f;
	mat4 scale_bias_matrix = 
		translate(mat4(), vec3(0.5f, 0.5f, 0.5f)) *
		scale(mat4(), vec3(0.5f, 0.5f, 0.5f));
	mat4 light_proj_matrix = ortho(-shadow_range, shadow_range, -shadow_range, shadow_range, 0.0f, 150.0f);
	// float viewportAspect = (float)width / (float)height;
	// mat4 light_proj_matrix = perspective(radians(100.0f), viewportAspect, 0.1f, 1000.0f);
	// mat4 light_view_matrix = lookAt(vec3(-31.75, 26.05, -97.72), vec3(-10, -13, -8), vec3(0.0f, 1.0f, 0.0f));
	mat4 light_view_matrix = lookAt(vec3(-31.75, 26.05, -97.72), vec3(0, 0, 0), vec3(0.0f, 1.0f, 0.0f));
	mat4 light_vp_matrix = light_proj_matrix * light_view_matrix;

	mat4 shadow_sbpv_matrix = scale_bias_matrix * light_vp_matrix;

	glUseProgram(depth.program);
	glBindFramebuffer(GL_FRAMEBUFFER, depthmap.fbo);
	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);

	// draw model
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(4.0f, 4.0f);
	for(int i=0; i<model.vaos.size(); i++)
	{
		glUniformMatrix4fv(uniforms.light.um4mvp, 1, GL_FALSE, value_ptr(light_vp_matrix * model_matrix));
		glBindVertexArray(model.vaos[i]);
		glDrawElements(GL_TRIANGLES, model.index_counts[i], GL_UNSIGNED_INT, 0);
	}
	// draw quad
	glUseProgram(quad.program);
	glUniformMatrix4fv(uniforms.quad.um4mvp, 1, GL_FALSE, value_ptr(light_vp_matrix * quad_matrix));
	glUniform1i(uniforms.quad.drawshadow, 0);
	glBindVertexArray(window.vaos[0]);
	glDrawArrays(GL_TRIANGLE_FAN,0,4);

	//===== draw sb
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDrawBuffer( GL_COLOR_ATTACHMENT0 );
	glViewport(0, 0, width, height);

	static const GLfloat gray[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	static const GLfloat ones[] = { 1.0f };
	glClearBufferfv(GL_COLOR, 0, gray);
	glClearBufferfv(GL_DEPTH, 0, ones);

	// draw sky box
	vec3 eye = vec3(0.0f, 0.0f, 0.0f);
	mat4 view_matrix = lookAt(view_position, view_position + view_direction, vec3(0.0f, 1.0f, 0.0f));
	mat4 inv_vp_matrix = inverse(proj_matrix * view_matrix);

	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.texture);

	glUseProgram(skybox.program);
	glBindVertexArray(skybox.vaos[0]);

	glUniformMatrix4fv(uniforms.skybox.inv_vp_matrix, 1, GL_FALSE, &inv_vp_matrix[0][0]);
	glUniform3fv(uniforms.skybox.eye, 1, &eye[0]);

	glDisable(GL_DEPTH_TEST);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glEnable(GL_DEPTH_TEST);


	// draw model
	mat4 shadow_matrix = shadow_sbpv_matrix * model_matrix;
	
	glUseProgram(model.program);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.texture);
	glUniform1i(uniforms.model.tex_cubemap, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthmap.tex);
	glUniform1i(uniforms.model.tex_shadow, 1);

	glUniformMatrix4fv(uniforms.model.um4m, 1, GL_FALSE, value_ptr(model_matrix));
	glUniformMatrix4fv(uniforms.model.um4v, 1, GL_FALSE, value_ptr(view_matrix));
	glUniformMatrix4fv(uniforms.model.um4p, 1, GL_FALSE, value_ptr(proj_matrix));
	glUniformMatrix4fv(uniforms.model.um4shadow, 1, GL_FALSE, value_ptr(shadow_matrix));
	
	for(int i=0; i<model.vaos.size(); i++)
	{
		glBindVertexArray(model.vaos[i]);
		glDrawElements(GL_TRIANGLES, model.index_counts[i], GL_UNSIGNED_INT, 0);
	}

	glutSwapBuffers();
}

void My_Reshape(int w, int h)
{
	width = w;
	height = h;
	glViewport(0, 0, width, height);
	float viewportAspect = (float)width / (float)height;
	proj_matrix = perspective(radians(80.0f), viewportAspect, 0.1f, 1000.0f);

	// setting sb
	bindFrameToTex(sb);
	bindFrameToTex(snoobj);
	bindFrameToTex(sobj);
}

void bindFrameToTex(Frame &frame)
{
	glDeleteRenderbuffers(1,&frame.rbo);
	glDeleteTextures(1,&frame.tex);

	glGenRenderbuffers( 1, &frame.rbo );
	glBindRenderbuffer( GL_RENDERBUFFER, frame.rbo );
	// glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, width, height );
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

	glGenTextures( 1, &frame.tex );
	glBindTexture( GL_TEXTURE_2D, frame.tex);

	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	glBindFramebuffer( GL_DRAW_FRAMEBUFFER, frame.fbo );
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, frame.rbo);
	// glFramebufferRenderbuffer( GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, frame.rbo );
	glFramebufferTexture2D( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frame.tex, 0 );
	
}

void My_MouseWheel(int button, int dir, int x, int y)
{
	view_position += view_direction * camera_speed *float(dir) * 3.0f;
}

void My_Timer(int val)
{
	glutPostRedisplay();
	glutTimerFunc(20, My_Timer, val);
}

int main(int argc, char *argv[])
{
#ifdef __APPLE__
    // Change working directory to source code path
    chdir(__FILEPATH__("/../Assets/"));
#endif
	// Initialize GLUT and GLEW, then create a window.
	////////////////////
	glutInit(&argc, argv);
#ifdef _MSC_VER
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#else
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(1400, 900);
	glutCreateWindow("Final"); // You cannot use OpenGL functions before this line; The OpenGL context must be created first by glutCreateWindow()!
#ifdef _MSC_VER
	glewInit();
#endif
    glPrintContextInfo();
	My_Init();

	// Register GLUT callback functions.
	glutDisplayFunc(My_Display);
	glutReshapeFunc(My_Reshape);
	glutKeyboardFunc(My_Keyboard);
	glutMouseFunc(My_Mouse);
	glutMotionFunc(My_MouseMove);
	glutMouseWheelFunc(My_MouseWheel);
	glutTimerFunc(timer_speed, My_Timer, 0); 

	// Enter main event loop.
	glutMainLoop();

	return 0;
}

char** loadShaderSource(const char* file)
{
    FILE* fp = fopen(file, "rb");
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *src = new char[sz + 1];
    fread(src, sizeof(char), sz, fp);
    src[sz] = '\0';
    char **srcp = new char*[1];
    srcp[0] = src;
    return srcp;
}

void freeShaderSource(char** srcp)
{
    delete[] srcp[0];
    delete[] srcp;
}

// // load a png image and return a TextureData structure with raw data
// // not limited to png format. works with any image format that is RGBA-32bit
TextureData loadImage(const char* const Filepath)
{
	TextureData texture;
	int n;
	stbi_set_flip_vertically_on_load(true);
	stbi_uc *data = stbi_load(Filepath, &texture.width, &texture.height, &n, 4);
	if (data != NULL)
	{
		texture.data = new unsigned char[texture.width * texture.height * 4 * sizeof(unsigned char)];
		memcpy(texture.data, data, texture.width * texture.height * 4 * sizeof(unsigned char));
		stbi_image_free(data);
	}
	return texture;
}

void linkProgram(GLuint program, const char* vertexFile, const char* fragmentFile)
{
	// Create customize shader by tell openGL specify shader type 

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	// Load shader file
	char** vertexShaderSource = loadShaderSource(vertexFile);
	char** fragmentShaderSource = loadShaderSource(fragmentFile);

	// Assign content of these shader files to those shaders we created before
    glShaderSource(vertexShader, 1, vertexShaderSource, NULL);
    glShaderSource(fragmentShader, 1, fragmentShaderSource, NULL);

	// Free the shader file string(won't be used any more)
	freeShaderSource(vertexShaderSource);
	freeShaderSource(fragmentShaderSource);

	// Compile these shaders
    glCompileShader(vertexShader);
    glCompileShader(fragmentShader);

	// Logging
	shaderLog(vertexShader);
    shaderLog(fragmentShader);

	// Assign the program we created before with these shaders
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

}

void loadModel()
{
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	string err;
	bool ret = tinyobj::LoadObj(shapes, materials, err, "nanosuit.obj");
	if(err.size()>0)
	{
		printf("Load Models Fail! Please check the solution path");
		cout << err;
		return;
	}
	printf("Load Models Success ! Shapes size %d Material size %d\n", shapes.size(), materials.size());

	for(int i=0; i<shapes.size(); i++)
	{
		GLuint vao;
		GLuint vbo;
		GLuint ebo;
		
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, (shapes[i].mesh.positions.size()+
										shapes[i].mesh.texcoords.size()+
										shapes[i].mesh.normals.size())*sizeof(float), NULL, GL_STATIC_DRAW);

		// position
		glBufferSubData(GL_ARRAY_BUFFER, 0, 
				shapes[i].mesh.positions.size() * sizeof(float), 
				&shapes[i].mesh.positions[0]);
		// texcoord
		glBufferSubData(GL_ARRAY_BUFFER, 
				shapes[i].mesh.positions.size() * sizeof(float), 
				shapes[i].mesh.texcoords.size() * sizeof(float), 
				&shapes[i].mesh.texcoords[0]);

		// normal
		glBufferSubData(GL_ARRAY_BUFFER, 
				shapes[i].mesh.positions.size() * sizeof(float) + shapes[i].mesh.texcoords.size() * sizeof(float), 
				shapes[i].mesh.normals.size() * sizeof(float), 
				&shapes[i].mesh.normals[0]);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void *) (shapes[i].mesh.positions.size() * sizeof(float)));
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void *) (shapes[i].mesh.positions.size() * sizeof(float) + shapes[i].mesh.texcoords.size() * sizeof(float)));
	
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		glGenBuffers(1, &ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, shapes[i].mesh.indices.size() * sizeof(unsigned int), shapes[i].mesh.indices.data(), GL_STATIC_DRAW);

		model.vaos.push_back(vao);		
		model.index_counts.push_back(shapes[i].mesh.indices.size());		
	}
}

void My_Mouse(int button, int state, int x, int y)
{
	
	mouseXY = vec2(x, y);
}

vec2 scaleMouse(vec2 mouse)
{
	float x = float(mouse.x) / width * 2.0f - 1.0f;
	float y = float(mouse.y) / height * 2.0f - 1.0f;
	return vec2(x, y);
}

vec3 projectToSphere(vec2 xy) {
    static const float sqrt2 = sqrtf(2.f);
    vec3 result;
    float d = length(xy);
    float size_=2;
    if (d < size_ * sqrt2 / 2.f) {
        // Inside sphere
        result.z = sqrtf(size_ * size_ - d*d);
    }
    else {
        // On hyperbola
        float t = size_ / sqrt2;
        result.z = t*t / d;
    }
    result.x = xy.x;
    result.y = xy.y;
    return normalize(result);
}

void My_MouseMove(int x,int y)
{

	vec2 oldMouse = scaleMouse(mouseXY);
	vec2 newMouse = scaleMouse(vec2(x, y));
	vec3 oldPosi = projectToSphere(oldMouse);
	vec3 newPosi = projectToSphere(newMouse);

	float anglex = newPosi.x - oldPosi.x;
	float angley = newPosi.y - oldPosi.y;

	vec3 right = cross(vec3(0.0, 1.0, 0.0), view_direction);
	vec3 normal = cross(right, view_direction);

	mat4 rotate_matrix = rotate(mat4(), anglex, normal);
	rotate_matrix = rotate(rotate_matrix, angley, right);
	view_direction = (vec4(view_direction, 0.0f) * rotate_matrix).xyz;

	mouseXY = vec2(x,y);
}

void My_Keyboard(unsigned char key, int x, int y)
{
	printf("Key %c is pressed at (%d, %d)\n", key, x, y);

	switch(key) {
		case 'q':
			rotate_angle += 10.0f;
		break;

		case 'e':
			rotate_angle += -10.0f;
		break;

		case 'i':
			showmode = (showmode+1) % 4;
		break;
	}
}