#include "../Externals/Include/Include.h"
#include "../Externals/Include/AntTweakBar.h"

using namespace glm;
using namespace std;

#define SHADOW_MAP_SIZE 4096

typedef struct _Material
{
	vec3 Ka; //ambient
	vec3 Kd; //diffuse
	vec3 Ks; //specular
	float Ns; // shininess
} Material;

typedef struct _Shader
{
	GLuint program;
	GLuint texture;
	GLuint texture2;
	vector<GLuint> vaos;
	vector<int> index_counts;

	vector<int> materialId;
	vector<Material> materials;
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
		GLint light_posi;

		GLint Ka;
		GLint Kd;
		GLint Ks;
		GLint Ns;

		GLint mode;
		GLint water_height;
    } model;
    struct
    {
        GLint um4mvp;
    } light;
    struct
    {
        GLint um4mvp;
		GLint tex_shadow;
		GLint drawshadow;
		GLint shadow_matrix;
    } quad;
	struct {
		GLint um4m;
		GLint um4v;
		GLint um4p;
		GLint moveFactor;
		GLint cameraPosition;
		GLint lightPosition;
	} water;
	struct
	{
		GLint um4p;
		GLint um4v;
		GLint cloud_time;
		GLint fsun;
		GLint cirrus;
		GLint cumulus;
	}sky;
	
} uniforms;

// loading function
char** loadShaderSource(const char* file);
void freeShaderSource(char** srcp);
void loadModel();
TextureData loadImage(const char* const Filepath);

// init

// opengl
void linkProgram(GLuint program, const char* vertexFile, const char* fragmentFile);
void bindFrameToTex(Frame &frame);

// trackball
vec3 projectToSphere(vec2 xy);
vec2 scaleMouse(vec2 mouse);

// draw
void drawModel();

// glut GUI
void My_Menu(int id);
void My_Init();
void TW_CALL My_Exit(void *);
void My_Display();
void My_Reshape(int w, int h);
void My_Keyboard(unsigned char key, int x, int y);
void My_Mouse(int button, int state, int x, int y);
void My_MouseMove(int x,int y);
void My_MouseWheel(int button, int dir, int x, int y);

// shader program
Shader screen;
Shader water;
Shader skybox;
Shader model;
Shader quad;
Shader depth;
Shader window;
Shader sky;
Frame depthmap;
Frame reflection;
Frame refraction;

// clock
unsigned int timer_speed = 20;

// camera
mat4 proj_matrix;
vec3 view_position;			 // a 3 dimension vector which represents how far did the ladybug should move
vec3 view_direction;
float rotate_angle=0.0f;
int width, height;

// track ball
vec2 mouseXY;
float camera_speed=1;

// clock
float time=0.0f;

// sky
float sun_time = 100.0f;
float sun_speed=500.0f;
float cloud_time = 0.0f;
float cloud_speed=300.0f;
float cumulus_clouds = 0.6f;
float cirrus_clouds = 0.4f;

// water
float water_height = 0.35f;
float wave_speed = 0.001f;
float move_factor = 0.0f;

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

void loadModel()
{
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	string err;
	bool ret = tinyobj::LoadObj(shapes, materials, err, "piano.obj");
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
//		glBufferSubData(GL_ARRAY_BUFFER, 
//				shapes[i].mesh.positions.size() * sizeof(float), 
//				shapes[i].mesh.texcoords.size() * sizeof(float), 
//				&shapes[i].mesh.texcoords[0]);

		// normal
//		glBufferSubData(GL_ARRAY_BUFFER, 
//				shapes[i].mesh.positions.size() * sizeof(float) + shapes[i].mesh.texcoords.size() * sizeof(float), 
//				shapes[i].mesh.normals.size() * sizeof(float), 
//				&shapes[i].mesh.normals[0]);
		glBufferSubData(GL_ARRAY_BUFFER, 
				shapes[i].mesh.positions.size() * sizeof(float) , 
				shapes[i].mesh.normals.size() * sizeof(float), 
				&shapes[i].mesh.normals[0]);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
//		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void *) (shapes[i].mesh.positions.size() * sizeof(float)));
//		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void *) (shapes[i].mesh.positions.size() * sizeof(float) + shapes[i].mesh.texcoords.size() * sizeof(float)));
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void *)(shapes[i].mesh.positions.size() * sizeof(float)));


		glEnableVertexAttribArray(0);
		//glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		glGenBuffers(1, &ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, shapes[i].mesh.indices.size() * sizeof(unsigned int), shapes[i].mesh.indices.data(), GL_STATIC_DRAW);

		model.vaos.push_back(vao);		
		model.index_counts.push_back(shapes[i].mesh.indices.size());	

		model.materialId.push_back(shapes[i].mesh.material_ids[0]);
	}

	for (int i = 0; i < materials.size(); i++) {

		Material _materail;
		_materail.Ka = vec3(materials[i].ambient[0], materials[i].ambient[1], materials[i].ambient[2]);
		_materail.Kd = vec3(materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2]);
		_materail.Ks = vec3(materials[i].specular[0], materials[i].specular[1], materials[i].specular[2]);
		_materail.Ns = materials[i].shininess;

		model.materials.push_back(_materail);
	}
}

// load a png image and return a TextureData structure with raw data
// not limited to png format. works with any image format that is RGBA-32bit
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

void My_Init()
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

	// ==== link shader ====

	// screen
	screen.program = glCreateProgram();
	linkProgram(screen.program, "screen.vs.glsl", "screen.fs.glsl");

	// water
	water.program = glCreateProgram();
	linkProgram(water.program, "water.vs.glsl", "water.fs.glsl");
	uniforms.water.um4m = glGetUniformLocation(water.program, "um4m");
	uniforms.water.um4v = glGetUniformLocation(water.program, "um4v");
	uniforms.water.um4p = glGetUniformLocation(water.program, "um4p");
	uniforms.water.moveFactor = glGetUniformLocation(water.program, "moveFactor");
	uniforms.water.cameraPosition = glGetUniformLocation(water.program, "cameraPosition");
	uniforms.water.lightPosition = glGetUniformLocation(water.program, "lightPosition");

	// skybox
	skybox.program = glCreateProgram();
	linkProgram(skybox.program, "skybox.vs.glsl", "skybox.fs.glsl");
	uniforms.skybox.inv_vp_matrix = glGetUniformLocation(skybox.program, "inv_vp_matrix");
	uniforms.skybox.eye = glGetUniformLocation(skybox.program, "eye");

	// model
	model.program = glCreateProgram();
	linkProgram(model.program, "model.vs.glsl", "model.fs.glsl");
	uniforms.model.um4p = glGetUniformLocation(model.program, "um4p");
    uniforms.model.um4m = glGetUniformLocation(model.program, "um4m");
    uniforms.model.um4v = glGetUniformLocation(model.program, "um4v");
    uniforms.model.um4shadow = glGetUniformLocation(model.program, "shadow_matrix");
    uniforms.model.tex_shadow = glGetUniformLocation(model.program, "tex_shadow");
    uniforms.model.tex_cubemap = glGetUniformLocation(model.program, "tex_cubemap");
    uniforms.model.light_posi = glGetUniformLocation(model.program, "light_pos");

	uniforms.model.Ka = glGetUniformLocation(model.program, "Ka");
	uniforms.model.Kd = glGetUniformLocation(model.program, "Kd");
	uniforms.model.Ks = glGetUniformLocation(model.program, "Ks");
	uniforms.model.Ns = glGetUniformLocation(model.program, "Ns");

	uniforms.model.mode = glGetUniformLocation(model.program, "mode");
	uniforms.model.water_height = glGetUniformLocation(model.program, "water_height");

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

	// sky
	sky.program = glCreateProgram();
	linkProgram(sky.program, "sky.vs.glsl", "sky.fs.glsl");
	uniforms.sky.um4p = glGetUniformLocation(sky.program, "P");
	uniforms.sky.um4v = glGetUniformLocation(sky.program, "V");
	uniforms.sky.fsun = glGetUniformLocation(sky.program, "fsun");
	uniforms.sky.cloud_time = glGetUniformLocation(sky.program, "cloud_time");
	uniforms.sky.cumulus = glGetUniformLocation(sky.program, "cumulus");
	uniforms.sky.cirrus = glGetUniformLocation(sky.program, "cirrus");

	// ==== load data to vao vbo ====

	// load skybox
	glUseProgram(skybox.program);
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
    
	GLuint skybox_vao;
	skybox.vaos.push_back(skybox_vao);
    glGenVertexArrays(1, &skybox.vaos[0]);

	// load model
	loadModel();

	// load sky 
	GLuint sky_vao, sky_vbo;
	sky.vaos.push_back(sky_vao);
    glGenVertexArrays(1, &sky.vaos[0]);
	glBindVertexArray(sky.vaos[0]);

	// load quad
	GLfloat quadVertices[] = {
	// Positions             // Texture
	-1000.0f, 0.0f, 1000.0f, 0.0f, 100.0f,
	-1000.0f, 0.0f, -1000.0f, 0.0f, 0.0f,
	1000.0f, 0.0f, -1000.0f, 100.0f, 0.0f,

	-1000.0f, 0.0f, 1000.0f, 0.0f, 100.0f,
	1000.0f, 0.0f, -1000.0f, 100.0f, 0.0f,
	1000.0f, 0.0f, 1000.0f, 100.0f, 100.0f
	};
	GLuint quad_vao ,quad_vbo;
	glGenVertexArrays(1, &quad_vao);
	glGenBuffers(1, &quad_vbo);
	glBindVertexArray(quad_vao);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glBindVertexArray(0);
	quad.vaos.push_back(quad_vao);

	TextureData texData = loadImage("quad_texture.png");

	glGenTextures(1, &quad.texture);
	glBindTexture(GL_TEXTURE_2D, quad.texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texData.width, texData.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texData.data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

	// load water 
	GLfloat waterVertices[] = {	
		// Positions                    // Texture
		-1000.0f, water_height, 1000.0f, 0.0f, 1000.0f,
		-1000.0f, water_height, -1000.0f, 0.0f, 0.0f,
		1000.0f, water_height, -1000.0f, 1000.0f, 0.0f,

		-1000.0f, water_height, 1000.0f, 0.0f, 1000.0f,
		1000.0f, water_height, -1000.0f, 1000.0f, 0.0f,
		1000.0f, water_height, 1000.0f, 1000.0f, 1000.0f
	};
	GLuint water_vao, water_vbo;
	glGenVertexArrays(1, &water_vao);
	glGenBuffers(1, &water_vbo);
	glBindVertexArray(water_vao);
	glBindBuffer(GL_ARRAY_BUFFER, water_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(waterVertices), &waterVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glBindVertexArray(0);
	water.vaos.push_back(water_vao);

	texData = loadImage("waterDUDV.png");
	glGenTextures(1, &water.texture);
	glBindTexture(GL_TEXTURE_2D, water.texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texData.width, texData.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texData.data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

	texData = loadImage("normalMap.png");
	glGenTextures(1, &water.texture2);
	glBindTexture(GL_TEXTURE_2D, water.texture2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texData.width, texData.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texData.data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

	// load screen
	GLfloat screenVertices[] = {	// Vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
	// Positions   // TexCoords
	-1.0f,  1.0f,  0.0f, 1.0f,
	-1.0f, -1.0f,  0.0f, 0.0f,
	 1.0f, -1.0f,  1.0f, 0.0f,

	-1.0f,  1.0f,  0.0f, 1.0f,
	 1.0f, -1.0f,  1.0f, 0.0f,
	 1.0f,  1.0f,  1.0f, 1.0f
	};
	GLuint screen_vao, screen_vbo;
	glGenVertexArrays(1, &screen_vao);
	glGenBuffers(1, &screen_vbo);
	glBindVertexArray(screen_vao);
	glBindBuffer(GL_ARRAY_BUFFER, screen_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(screenVertices), &screenVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)(2 * sizeof(GLfloat)));
	glBindVertexArray(0);
	screen.vaos.push_back(screen_vao);

	// refletion & refraction frameobject
	glGenFramebuffers(1, &reflection.fbo);
	glGenFramebuffers(1, &refraction.fbo);

	// shadow frameobject
	glGenFramebuffers(1, &depthmap.fbo);

	glGenTextures(1, &depthmap.tex);
	glBindTexture(GL_TEXTURE_2D, depthmap.tex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 0,  GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	glBindFramebuffer(GL_FRAMEBUFFER, depthmap.fbo);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthmap.tex, 0);

	// ==== setup viewing position and rotation ====
	view_position = vec3(0.0f, 0.0f, 10.0f);
	view_direction = normalize(vec3(0.0f, -1.0f, -2.0f));	

	// ==== GUI setup ====
	TwInit(TW_OPENGL_CORE, NULL);
	TwBar *myBar;
	myBar = TwNewBar("TweakBar");
	TwDefine(" GLOBAL help='This example shows how to integrate AntTweakBar with GLUT and OpenGL.' "); // Message added to the help bar.
    TwDefine(" TweakBar size='200 400' color='96 216 224' "); // change default tweak bar size and color

	TwAddVarRW(myBar, "Cirrus", TW_TYPE_FLOAT, &cirrus_clouds, 
				"min=0 max=1.0 step=0.05 group=Sky help='dence of Cirrus cloud'");
	TwAddVarRW(myBar, "Cumulus", TW_TYPE_FLOAT, &cumulus_clouds, 
				"min=0 max=1.0 step=0.05 group=Sky help='dence of Cumulus cloud'");

    TwAddVarRW(myBar, "Sun Speed", TW_TYPE_FLOAT, &sun_speed, 
               " min=-2000 max=4000.0 step=100 help='Change the speed of sun' group=SkyClock");
    TwAddVarRW(myBar, "Cloud Speed", TW_TYPE_FLOAT, &cloud_speed, 
               " min=0 max=1000.0 step=50 help='Change the speed of cloud' group=SkyClock");
    TwAddVarRW(myBar, "Sun Time", TW_TYPE_FLOAT, &sun_time, 
               " min=0 step=0.5 help='Sun Timer' group=SkyClock");
    TwAddVarRW(myBar, "Cloud Time", TW_TYPE_FLOAT, &cloud_time, 
               " min=0 step=0.5 help='Cloud timer' group=SkyClock");
	TwAddButton(myBar, "Exit", My_Exit, NULL, " ");
}

void My_Display()
{
	// ==== model transform ====
	mat4 view_matrix = lookAt(view_position, view_position + view_direction, vec3(0.0f, 1.0f, 0.0f));
	mat4 translation_matrix = translate(mat4(), vec3(0.0f, 0.0f, 0.0f));
	mat4 rotate_matrix = rotate(mat4(), radians(30.0f), vec3(0.0, 1.0, 0.0));
	mat4 scale_matrix = scale(mat4(), vec3(4.0f, 4.5f, 4.0f));
	mat4 model_matrix =  translation_matrix * rotate_matrix * scale_matrix;
	model_matrix = rotate(model_matrix, radians(rotate_angle), vec3(0.0, 1.0, 0.0));

	// sun posi
	vec3 sun_posi = vec3(0.0, sin(sun_time * 0.01), cos(sun_time * 0.01));

	// ===== draw shadow map pass ====
	const float shadow_range = 20.0f;
	mat4 scale_bias_matrix = 
		translate(mat4(), vec3(0.5f, 0.5f, 0.5f)) *
		scale(mat4(), vec3(0.5f, 0.5f, 0.5f));
	mat4 light_proj_matrix = ortho(-shadow_range, shadow_range, -shadow_range, shadow_range, 0.0f, 200.0f);
//	mat4 light_view_matrix = lookAt(vec3(-31.75, 26.05, -97.72), vec3(0, 0, 0), vec3(0.0f, 1.0f, 0.0f));
	// mat4 light_view_matrix = lookAt(vec3(10.0f, 10.0f, 6.0f), vec3(0, 0, 0), vec3(0.0f, 1.0f, 0.0f));
	mat4 light_view_matrix = lookAt(sun_posi*5.0f, vec3(0, 0, 0), vec3(0.0f, 1.0f, 0.0f));
	mat4 light_vp_matrix = light_proj_matrix * light_view_matrix;

	mat4 shadow_sbpv_matrix = scale_bias_matrix * light_vp_matrix;
	mat4 shadow_matrix = shadow_sbpv_matrix * model_matrix;

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
	glDisable(GL_POLYGON_OFFSET_FILL);

	static const GLfloat gray[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	static const GLfloat ones[] = { 1.0f };
	
	glEnable(GL_CLIP_DISTANCE0);

	//==== draw to reflection texture ====
	float distance = 2.0f * (view_position.y - (water_height));
	view_position.y -= distance;
	view_direction.y = -view_direction.y;


	view_matrix = lookAt(view_position, view_position + view_direction, vec3(0.0f, 1.0f, 0.0f));

	glBindFramebuffer(GL_FRAMEBUFFER, reflection.fbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glViewport(0, 0, width, height);
	glClearBufferfv(GL_COLOR, 0, gray);
	glClearBufferfv(GL_DEPTH, 0, ones);

	// draw sky
	glUseProgram(sky.program);
	glUniformMatrix4fv(uniforms.sky.um4p, 1, GL_FALSE, value_ptr(proj_matrix));
	glUniformMatrix4fv(uniforms.sky.um4v, 1, GL_FALSE, value_ptr(view_matrix));
	glUniform3fv(uniforms.sky.fsun, 1, value_ptr(sun_posi));
	glUniform1f(uniforms.sky.cloud_time, cloud_time);
	glUniform1f(uniforms.sky.cirrus, cirrus_clouds);
	glUniform1f(uniforms.sky.cumulus, cumulus_clouds);

	glBindVertexArray(sky.vaos[0]);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	// draw model
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
	// glUniform3fv(uniforms.model.light_posi, 1, value_ptr(sun_posi));
	glUniform1i(uniforms.model.mode, 0);
	glUniform1f(uniforms.model.water_height, water_height);

	for (int i = 0; i < model.vaos.size(); i++)
	{
		//material
		int mid = model.materialId[i];
		glUniform3fv(uniforms.model.Ka, 1, value_ptr(model.materials[mid].Ka));
		glUniform3fv(uniforms.model.Kd, 1, value_ptr(model.materials[mid].Kd));
		glUniform3fv(uniforms.model.Ks, 1, value_ptr(model.materials[mid].Ks));
		glUniform1f(uniforms.model.Ns, model.materials[mid].Ns);

		glBindVertexArray(model.vaos[i]);
		glDrawElements(GL_TRIANGLES, model.index_counts[i], GL_UNSIGNED_INT, 0);
	}

	view_position.y += distance;
	view_direction.y = -view_direction.y;
	view_matrix = lookAt(view_position, view_position + view_direction, vec3(0.0f, 1.0f, 0.0f));
	
	//===== draw to refraction texture ====
	glBindFramebuffer(GL_FRAMEBUFFER, refraction.fbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glViewport(0, 0, width, height);

	glClearBufferfv(GL_COLOR, 0, gray);
	glClearBufferfv(GL_DEPTH, 0, ones);
	// draw quad (floor)
	glUseProgram(quad.program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthmap.tex);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, quad.texture);

	glUniformMatrix4fv(uniforms.quad.um4mvp, 1, GL_FALSE, value_ptr(proj_matrix * view_matrix* model_matrix));
	glUniform1i(uniforms.quad.drawshadow, 1);
	glUniformMatrix4fv(uniforms.quad.shadow_matrix, 1, GL_FALSE, value_ptr(shadow_matrix));
	glUniform1i(uniforms.quad.tex_shadow, 0);

	glBindVertexArray(quad.vaos[0]);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	// draw model
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
	glUniform1i(uniforms.model.mode, 1);
	glUniform1f(uniforms.model.water_height, water_height);

	for (int i = 0; i < model.vaos.size(); i++){
		//material
		int mid = model.materialId[i];
		glUniform3fv(uniforms.model.Ka, 1, value_ptr(model.materials[mid].Ka));
		glUniform3fv(uniforms.model.Kd, 1, value_ptr(model.materials[mid].Kd));
		glUniform3fv(uniforms.model.Ks, 1, value_ptr(model.materials[mid].Ks));
		glUniform1f(uniforms.model.Ns, model.materials[mid].Ns);

		glBindVertexArray(model.vaos[i]);
		glDrawElements(GL_TRIANGLES, model.index_counts[i], GL_UNSIGNED_INT, 0);
	}

	glDisable(GL_CLIP_DISTANCE0);

	//===== draw to frame ====
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDrawBuffer( GL_COLOR_ATTACHMENT0 );
	glViewport(0, 0, width, height);

	glClearBufferfv(GL_COLOR, 0, gray);
	glClearBufferfv(GL_DEPTH, 0, ones);

	// draw sky box
	vec3 eye = vec3(0.0f, 0.0f, 0.0f);
	mat4 inv_vp_matrix = inverse(proj_matrix * view_matrix);

	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.texture);

	glUseProgram(skybox.program);
	glBindVertexArray(skybox.vaos[0]);

	glUniformMatrix4fv(uniforms.skybox.inv_vp_matrix, 1, GL_FALSE, &inv_vp_matrix[0][0]);
	glUniform3fv(uniforms.skybox.eye, 1, &eye[0]);

	glDisable(GL_DEPTH_TEST);
//	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glEnable(GL_DEPTH_TEST);

	// draw sky
	glUseProgram(sky.program);
	glUniformMatrix4fv(uniforms.sky.um4p, 1, GL_FALSE, value_ptr(proj_matrix));
  	glUniformMatrix4fv(uniforms.sky.um4v, 1, GL_FALSE, value_ptr(view_matrix));
  	glUniform3fv(uniforms.sky.fsun, 1, value_ptr(sun_posi));
  	glUniform1f(uniforms.sky.cloud_time, cloud_time);
  	glUniform1f(uniforms.sky.cirrus, cirrus_clouds);
  	glUniform1f(uniforms.sky.cumulus, cumulus_clouds);
	
	glBindVertexArray(sky.vaos[0]);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	

	// draw quad (floor)
	glUseProgram(quad.program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthmap.tex);	
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, quad.texture);

	glUniformMatrix4fv(uniforms.quad.um4mvp, 1, GL_FALSE, value_ptr(proj_matrix * view_matrix* model_matrix));
	glUniform1i(uniforms.quad.drawshadow, 1);
	glUniformMatrix4fv(uniforms.quad.shadow_matrix, 1, GL_FALSE, value_ptr(shadow_matrix));
	glUniform1i(uniforms.quad.tex_shadow, 0);

	glBindVertexArray(quad.vaos[0]);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	// draw water
	glUseProgram(water.program);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, reflection.tex);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, refraction.tex);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, water.texture);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, water.texture2);

	glUniformMatrix4fv(uniforms.water.um4m, 1, GL_FALSE, value_ptr(model_matrix));
	glUniformMatrix4fv(uniforms.water.um4v, 1, GL_FALSE, value_ptr(view_matrix));
	glUniformMatrix4fv(uniforms.water.um4p, 1, GL_FALSE, value_ptr(proj_matrix));
	glUniform1f(uniforms.water.moveFactor, move_factor);
	glUniform3fv(uniforms.water.cameraPosition, 1, value_ptr(view_position));
	glUniform3f(uniforms.water.lightPosition, 10.0f, 10.0f, 6.0f);

	glBindVertexArray(water.vaos[0]);
	glDrawArrays(GL_TRIANGLES, 0, 6);



	// draw model
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
	glUniform3fv(uniforms.model.light_posi, 1, value_ptr(sun_posi * 10.0f));
	// vec3 light = vec3(0.0, 10.0, 0.0);
	// glUniform3fv(uniforms.model.light_posi, 1, value_ptr(light));

	for(int i=0; i<model.vaos.size(); i++)
	{
		//material
		int mid = model.materialId[i];
		glUniform3fv(uniforms.model.Ka, 1, value_ptr(model.materials[mid].Ka));
		glUniform3fv(uniforms.model.Kd, 1, value_ptr(model.materials[mid].Kd));
		glUniform3fv(uniforms.model.Ks, 1, value_ptr(model.materials[mid].Ks));
		glUniform1f(uniforms.model.Ns, model.materials[mid].Ns);

		glBindVertexArray(model.vaos[i]);
		glDrawElements(GL_TRIANGLES, model.index_counts[i], GL_UNSIGNED_INT, 0);
	}
	
	// draw GUI
	TwDraw();
	/*
	 // draw on screen //
	{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // white
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(screen.program);
	glBindVertexArray(screen.vaos[0]);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, reflection.tex);

	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
	glUseProgram(0);
	}
	*/
	glutSwapBuffers();
}

void My_Reshape(int w, int h)
{
	width = w;
	height = h;
	glViewport(0, 0, width, height);
	float viewportAspect = (float)width / (float)height;
	proj_matrix = perspective(radians(80.0f), viewportAspect, 0.1f, 1000.0f);
	TwWindowSize(width, height);

	bindFrameToTex(reflection);
	bindFrameToTex(refraction);
}

void TW_CALL My_Exit(void *)
{
	exit(0);
}

void My_Timer(int val)
{
	glutPostRedisplay();
	time += timer_speed * 0.000001f;
	sun_time += sun_speed * timer_speed * 0.000001f;
	cloud_time += cloud_speed * timer_speed * 0.000001f;

	//water
	move_factor += wave_speed;
	if(move_factor>=1)  move_factor = 0;

	glutTimerFunc(20, My_Timer, val);
}

void My_Mouse(int button, int state, int x, int y)
{
	if( !TwEventMouseButtonGLUT(button, state, x, y))
	{
		mouseXY = vec2(x, y);
	}
}

void My_MouseMove(int x,int y)
{
	if( !TwEventMouseMotionGLUT(x, y))
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
}

void My_Keyboard(unsigned char key, int x, int y)
{
	if( !TwEventKeyboardGLUT(key, x, y))
	{
		printf("Key %c is pressed at (%d, %d)\n", key, x, y);

		switch(key) {
			case 'q':
				rotate_angle += 10.0f;
			break;

			case 'e':
				rotate_angle += -10.0f;
			break;

			case 'w':
				view_position += vec3(view_direction.x, 0.0, view_direction.z) * camera_speed;
			break;

			case 's':
				view_position += -vec3(view_direction.x, 0.0, view_direction.z) * camera_speed;
			break;

			case 'd':
				view_position += vec3(-view_direction.z, 0.0, view_direction.x) * camera_speed;
			break;

			case 'a':
				view_position += vec3(view_direction.z, 0.0, -view_direction.x) * camera_speed;
			break;

			case 32:
				view_position += vec3(0, 1, 0) * camera_speed;
			break;

			case 'z':
				view_position += vec3(0, -1, 0) * camera_speed;
			break;
		}

	}
}

void My_MouseWheel(int button, int dir, int x, int y)
{
	view_position += view_direction * camera_speed * float(dir);
}

void My_Terminate(void)
{
    TwTerminate();
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
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(1400, 900);
	glutCreateWindow("Final"); // You cannot use OpenGL functions before this line; The OpenGL context must be created first by glutCreateWindow()!
#ifdef _MSC_VER
	glewInit();
#endif
    glPrintContextInfo();
	My_Init();

	//Tw Event 
	// Set GLUT event callbacks
    // - Directly redirect GLUT mouse button events to AntTweakBar
    glutMouseFunc((GLUTmousebuttonfun)TwEventMouseButtonGLUT);
    // - Directly redirect GLUT mouse motion events to AntTweakBar
    glutMotionFunc((GLUTmousemotionfun)TwEventMouseMotionGLUT);
    // - Directly redirect GLUT mouse "passive" motion events to AntTweakBar (same as MouseMotion)
    glutPassiveMotionFunc((GLUTmousemotionfun)TwEventMouseMotionGLUT);
    // - Directly redirect GLUT key events to AntTweakBar
    glutKeyboardFunc((GLUTkeyboardfun)TwEventKeyboardGLUT);
    // - Directly redirect GLUT special key events to AntTweakBar
    glutSpecialFunc((GLUTspecialfun)TwEventSpecialGLUT);
    // - Send 'glutGetModifers' function pointer to AntTweakBar;
    //   required because the GLUT key event functions do not report key modifiers states.
    TwGLUTModifiersFunc(glutGetModifiers);


	// // Register GLUT callback functions.
	glutDisplayFunc(My_Display);
	glutReshapeFunc(My_Reshape);
	glutKeyboardFunc(My_Keyboard);
	glutMouseFunc(My_Mouse);
	glutMotionFunc(My_MouseMove);
	glutMouseWheelFunc(My_MouseWheel);
	glutTimerFunc(timer_speed, My_Timer, 0); 
	atexit(My_Terminate);

	// Enter main event loop.
	glutMainLoop();

	return 0;
}

