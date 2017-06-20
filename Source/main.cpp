#include "../Externals/Include/Include.h"

#define MENU_TIMER_START 1
#define MENU_TIMER_STOP 2
#define MENU_EXIT 3

GLubyte timer_cnt = 0;
bool timer_enabled = true;
unsigned int timer_speed = 16;
float movementSpeed = 500;
const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;

using namespace glm;
using namespace std;

GLuint program;
GLuint skyProgram;
GLuint regularProgram;
GLuint depthProgram;
GLuint waterProgram;

mat4 model;
mat4 view;
mat4 proj_matrix;

vec3 light_eye;
vec3 light_up;
vec3 light_center;
vec3 default_ambient = vec3(0.7);
mat4 light_view_matrix;
mat4 light_proj_matrix;

GLint um4mv;
GLint um4p;
GLint um4shadow;
GLint uv3lightpos;
GLint depth_um4mv;
GLint depth_um4p;

GLuint tex;
GLuint shadowMap;
GLuint alpha;
GLuint Ka;
GLuint Kd;
GLuint Ks;
GLuint skytex;
GLuint skyeye;
GLuint skyvp;

vec3 cam_eye;
vec3 cam_up;
vec3 cam_center;
vec3 forward_vec;
vec3 modle_pos;
vec2 window_size;

GLuint window_vao;
GLuint window_buffer;
GLuint FBO;
GLuint depthRBO;
GLuint FBODataTexture;
GLuint noiseTexture;
GLuint depth_fbo;
GLuint depth_tex;

GLuint water_tex;
GLint water_um4mv;
GLint water_um4p;

static const GLfloat window_positions[] =
{
	1.0f,-1.0f,1.0f,0.0f,
	-1.0f,-1.0f,0.0f,0.0f,
	-1.0f,1.0f,0.0f,1.0f,
	1.0f,1.0f,1.0f,1.0f
};

using namespace glm;
using namespace std;

void My_Reshape(int width, int height);

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

void shaderLog(GLuint shader)
{
	GLint isCompiled = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character
		GLchar* errorLog = new GLchar[maxLength];
		glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);

		printf("%s\n", errorLog);
		delete[] errorLog;
	}
}

// define a simple data structure for storing texture image raw data
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

typedef struct Shape
{
	GLuint vao;
	GLuint vbo_position;
	GLuint vbo_normal;
	GLuint vbo_texcoord;
	GLuint ibo;
	int drawCount;
	int materialID;
} Shape;

typedef struct Material
{
	GLuint diffuse_tex;
	vec3 Ka;
	vec3 Kd;
	vec3 Ks;
	float a;
} Material;

typedef struct Scene 
{
	Shape *shapes;
	Material *materials;
	unsigned int meshNum;
	unsigned int materialNum;
}Scene;

TextureData WaterTextureData[100];
int WaterTimer = 0;

// load a png image and return a TextureData structure with raw data
// not limited to png format. works with any image format that is RGBA-32bit
TextureData loadPNG(const char* const pngFilepath)
{
    TextureData texture;
    int components;

    // load the texture with stb image, force RGBA (4 components required)
    stbi_uc *data = stbi_load(pngFilepath, &texture.width, &texture.height, &components, 4);
	printf("%s: ", pngFilepath);
    // is the image successfully loaded?
    if (data != NULL)
	{
		printf("success\n");
        // copy the raw data
        size_t dataSize = texture.width * texture.height * 4 * sizeof(unsigned char);
        texture.data = new unsigned char[dataSize];
        memcpy(texture.data, data, dataSize);

        // mirror the image vertically to comply with OpenGL convention
        for (size_t i = 0; i < texture.width; ++i)
        {
            for (size_t j = 0; j < texture.height / 2; ++j)
            {
                for (size_t k = 0; k < 4; ++k)
                {
                    size_t coord1 = (j * texture.width + i) * 4 + k;
                    size_t coord2 = ((texture.height - j - 1) * texture.width + i) * 4 + k;
                    std::swap(texture.data[coord1], texture.data[coord2]);
                }
            }
        }

        // release the loaded image
        stbi_image_free(data);
	}
	else { printf("failed\n"); }

    return texture;
}

Scene scene2_1;
Scene lakecity;
Scene city;
Scene organodron;
Scene scidowntown;
Scene castle;

int mode = 0;

const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);
Shape skybox;
Material skyboxTex;

static const GLfloat screenQuad[] = {
	-1.0f, -1.0f, 1.0f,
	-1.0f, 1.0f, 1.0f,
	1.0f, -1.0f, 1.0f,
	1.0f,  1.0f, 1.0f
};

unsigned int screenQuadIndex[] = {
	0,1,2,3
};

Scene LoadScene(const char* const filePath, const char* const directory) {
	const aiScene* scene = aiImportFile(filePath, aiProcessPreset_TargetRealtime_MaxQuality);
	Scene sc;
	sc.shapes = new Shape[scene->mNumMeshes];
	sc.materials = new Material[scene->mNumMaterials];
	sc.meshNum = scene->mNumMeshes;
	sc.materialNum = scene->mNumMaterials;

	for (unsigned int i = 0; i < scene->mNumMaterials; ++i){
		aiMaterial *material = scene->mMaterials[i];
		aiString texturePath;
		
		if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == aiReturn_SUCCESS){
			// load width, height and data from texturePath.C_Str();
			aiString file = aiString(directory);
			file.Append(texturePath.C_Str());
			TextureData textureData = loadPNG(file.C_Str());
			glGenTextures(1, &sc.materials[i].diffuse_tex);
			glBindTexture(GL_TEXTURE_2D, sc.materials[i].diffuse_tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureData.width, textureData.height, 0,GL_RGBA, GL_UNSIGNED_BYTE, textureData.data);
			glGenerateMipmap(GL_TEXTURE_2D);
			sc.materials[i].a = 1.0;
		}
		else
		{
			// load some default image as default_diffuse_tex
			TextureData textureData = loadPNG("default.png");
			glGenTextures(1, &sc.materials[i].diffuse_tex);
			glBindTexture(GL_TEXTURE_2D, sc.materials[i].diffuse_tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureData.width, textureData.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData.data);
			glGenerateMipmap(GL_TEXTURE_2D);
			sc.materials[i].a = 1.0;
		}

		aiColor3D a;
		if (material->Get(AI_MATKEY_COLOR_AMBIENT, a) == aiReturn_SUCCESS) {
			if (a.r == 0 && a.g == 0 && a.b == 0) {
				sc.materials[i].Ka = default_ambient;
			}
			else {
				sc.materials[i].Ka = vec3(a.r, a.g, a.b);
			}
		}
		else {
			sc.materials[i].Ka = vec3(1, 1, 1);
		}

		aiColor3D d;
		if (material->Get(AI_MATKEY_COLOR_DIFFUSE, d) == aiReturn_SUCCESS) {
			sc.materials[i].Kd = vec3(d.r, d.g, d.b);
		}
		else {
			sc.materials[i].Kd = vec3(1, 1, 1);
		}

		aiColor3D s;
		if (material->Get(AI_MATKEY_COLOR_SPECULAR, s) == aiReturn_SUCCESS) {
			sc.materials[i].Ks = vec3(s.r, s.g, s.b);
		}
		else {
			sc.materials[i].Ks = vec3(1, 1, 1);
		}
	}

	for (unsigned int i = 0; i< scene->mNumMeshes; ++i){
		aiMesh *mesh = scene->mMeshes[i];
		glGenVertexArrays(1, &sc.shapes[i].vao);
		glBindVertexArray(sc.shapes[i].vao);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		
		// create 3 vbos to hold data
		glGenBuffers(1, &sc.shapes[i].vbo_position);
		glGenBuffers(1, &sc.shapes[i].vbo_normal);
		glGenBuffers(1, &sc.shapes[i].vbo_texcoord);
		
		float *position = new float[3 * mesh->mNumVertices];
		float *normal = new float[3 * mesh->mNumVertices];
		float *texcoord = new float[2 * mesh->mNumVertices];

		for (unsigned int v = 0; v < mesh->mNumVertices; ++v){
			const aiVector3D* pos = &(mesh->mVertices[v]);
			const aiVector3D* nor = mesh->HasNormals()? &(mesh->mNormals[v]) : &Zero3D;
			const aiVector3D* texCoor = mesh->HasTextureCoords(0) ? &(mesh->mTextureCoords[0][v]) : &Zero3D;

			// mesh->mVertices[v][0~2] => position
			position[3 * v + 0] = pos->x;
			position[3 * v + 1] = pos->y;
			position[3 * v + 2] = pos->z;

			// mesh->mNormals[v][0~2] => normal
			normal[3 * v + 0] = nor->x;
			normal[3 * v + 1] = nor->y;
			normal[3 * v + 2] = nor->z;
			
			// mesh->mTextureCoords[0][v][0~1] => texcoord
			texcoord[2 * v + 0] = texCoor->x;
			texcoord[2 * v + 1] = texCoor->y;

		}
		glBindBuffer(GL_ARRAY_BUFFER, sc.shapes[i].vbo_position);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * mesh->mNumVertices * 3, position, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glBindBuffer(GL_ARRAY_BUFFER, sc.shapes[i].vbo_texcoord);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * mesh->mNumVertices * 2, texcoord, GL_STATIC_DRAW);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
		glBindBuffer(GL_ARRAY_BUFFER, sc.shapes[i].vbo_normal);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * mesh->mNumVertices * 3, normal, GL_STATIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);

		delete position;
		delete normal;
		delete texcoord;

		// create 1 iboto hold data
		glGenBuffers(1, &sc.shapes[i].ibo);

		unsigned int *index = new unsigned int[3 * mesh->mNumFaces];

		for (unsigned int f = 0; f < mesh->mNumFaces; ++f){
			// mesh->mFaces[f].mIndices[0~2] => index
			if (mesh->mFaces[f].mIndices[0] >= mesh->mNumVertices || mesh->mFaces[f].mIndices[1] >= mesh->mNumVertices || mesh->mFaces[f].mIndices[2] >= mesh->mNumVertices) {
				continue;
			}
			index[3 * f + 0] = mesh->mFaces[f].mIndices[0];
			index[3 * f + 1] = mesh->mFaces[f].mIndices[1];
			index[3 * f + 2] = mesh->mFaces[f].mIndices[2];

		}
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sc.shapes[i].ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * mesh->mNumFaces * 3, index, GL_STATIC_DRAW);
		delete index;

		// glVertexAttribPointer/ glEnableVertexArraycalls¡K
		sc.shapes[i].materialID = mesh->mMaterialIndex;
		sc.shapes[i].drawCount = mesh->mNumFaces * 3;
		glBindVertexArray(0);
	}
	aiReleaseImport(scene);

	return sc;
}


void LoadSkybox() {
	TextureData textureData;

	glGenTextures(1, &skyboxTex.diffuse_tex);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex.diffuse_tex);
	const char* const face[6] = {
		"./clouds1/clouds1_east.bmp",
		"./clouds1/clouds1_west.bmp",
		"./clouds1/clouds1_up.bmp",
		"./clouds1/clouds1_down.bmp",
		"./clouds1/clouds1_south.bmp",
		"./clouds1/clouds1_north.bmp"
	};

	for (unsigned i = 0; i < 6; i++) {
		textureData = loadPNG(face[i]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, textureData.width, textureData.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData.data);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	skyboxTex.a = 1.0;

	glGenVertexArrays(1, &skybox.vao);
	glBindVertexArray(skybox.vao);
	glEnableVertexAttribArray(0);
	glGenBuffers(1, &skybox.vbo_position);
	glBindBuffer(GL_ARRAY_BUFFER, skybox.vbo_position);
	glBufferData(GL_ARRAY_BUFFER, sizeof(screenQuad), screenQuad, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glGenBuffers(1, &skybox.ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skybox.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(screenQuadIndex), screenQuadIndex, GL_STATIC_DRAW);
	skybox.drawCount = sizeof(screenQuadIndex);
	glBindVertexArray(0);
}

void LoadWater() {
	char str[] = "./water/color/water_079_c_0001.jpg"; //27 28 29
	for (int i = 1; i <= 100; i++) {
		int first = i / 100, second = (i / 10) % 10, third = i % 10;
		str[27] = first + '0'; str[28] = second + '0'; str[29] = third + '0';
		WaterTextureData[i-1] = loadPNG(str);
	}
	
}

void My_Init()
{
    glClearColor(0.0f, 0.6f, 0.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	
	scene2_1 = LoadScene("./Airbus A310/Airbus A310.obj", "./Airbus A310");
	//scene2_1 = LoadScene("./water/flow.obj", "./water/");
	//scene2_1 = LoadScene("./Venice/venice.obj", "./Venice/");
	//scene2_1 = LoadScene("./Medieval/Medieval_City.obj", "./Medieval/");
	//scene2_1 = LoadScene("./Damaged Downtown/Downtown_Damage_0.obj", "./Damaged Downtown/");
	//scene2_1 = LoadScene("./obj/obj.obj", "./obj");
	//scene2_1 = LoadScene("./Sirus5 Colonial City/sirus city.obj", "./Sirus5 Colonial City");
	//scene2_1 = LoadScene("./colony sector/colony sector.obj", "./colony sector");
	//scene2_1 = LoadScene("./The City/The City.obj", "./The City/");
	//scene2_1 = LoadScene("./castle/castle.obj", "./castle/");
	//scene2_1 = LoadScene("./Paris/Paris2010_0.obj", "./Paris/");
	//scene2_1 = LoadScene("./serpertine city/serpertine city.obj", "./serpertine city/");
	//scene2_1 = LoadScene("./scifi tropical city/Sci-fi Tropical city.obj", "./scifi tropical city/");
	//scene2_1 = LoadScene("./crytek-sponza/sponza.obj", "./crytek-sponza/");
	//scene2_1 = LoadScene("./Lakecity/Lakecity.obj", "./Lakecity/");
	//city = LoadScene("./The City/The City.obj", "./The City/");
	//organodron = LoadScene("./Organodron City/Organodron City.obj", "./Organodron City/");
	//scidowntown = LoadScene("./scifi dowtown scenery/scifi dowtown scenery.obj", "./scifi dowtown scenery/");
	//castle = LoadScene("./castle/castle.obj", "./castle/");

	//LoadWater
	LoadWater();

	LoadSkybox();
	
	program = glCreateProgram();
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	char** vertexShaderSource = loadShaderSource("vertex.vs.glsl");
	char** fragmentShaderSource = loadShaderSource("fragment.fs.glsl");
	glShaderSource(vertexShader, 1, vertexShaderSource, NULL);
	glShaderSource(fragmentShader, 1, fragmentShaderSource, NULL);
	freeShaderSource(vertexShaderSource);
	freeShaderSource(fragmentShaderSource);
	glCompileShader(vertexShader);
	glCompileShader(fragmentShader);
	shaderLog(vertexShader);
	shaderLog(fragmentShader);
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);
	um4mv = glGetUniformLocation(program, "um4mv");
	um4p = glGetUniformLocation(program, "um4p");
	um4shadow = glGetUniformLocation(program, "um4shadow");
	uv3lightpos = glGetUniformLocation(program, "uv3lightpos");
	tex = glGetUniformLocation(program, "tex");
	shadowMap = glGetUniformLocation(program, "shadowMap");
	alpha = glGetUniformLocation(program, "alpha");
	Ka = glGetUniformLocation(program, "Ka");
	Kd = glGetUniformLocation(program, "Kd");
	Ks = glGetUniformLocation(program, "Ks");
	
	skyProgram = glCreateProgram();
	GLuint skyVertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint skyFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	char** skyVertexShaderSource = loadShaderSource("skyvertex.vs.glsl");
	char** skyFragmentShaderSource = loadShaderSource("skyfragment.fs.glsl");
	glShaderSource(skyVertexShader, 1, skyVertexShaderSource, NULL);
	glShaderSource(skyFragmentShader, 1, skyFragmentShaderSource, NULL);
	freeShaderSource(skyVertexShaderSource);
	freeShaderSource(skyFragmentShaderSource);
	glCompileShader(skyVertexShader);
	glCompileShader(skyFragmentShader);
	shaderLog(skyVertexShader);
	shaderLog(skyFragmentShader);
	glAttachShader(skyProgram, skyVertexShader);
	glAttachShader(skyProgram, skyFragmentShader);
	glLinkProgram(skyProgram);
	skytex = glGetUniformLocation(skyProgram, "skybox");
	skyeye = glGetUniformLocation(skyProgram, "eye");
	skyvp = glGetUniformLocation(skyProgram, "vp_matrix");
	
	regularProgram = glCreateProgram();
	GLuint regularVertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint regularFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	char** regularVertexShaderSource = loadShaderSource("regular.vs.glsl");
	char** regularFragmentShaderSource = loadShaderSource("regular.fs.glsl");
	glShaderSource(regularVertexShader, 1, regularVertexShaderSource, NULL);
	glShaderSource(regularFragmentShader, 1, regularFragmentShaderSource, NULL);
	freeShaderSource(regularVertexShaderSource);
	freeShaderSource(regularFragmentShaderSource);
	glCompileShader(regularVertexShader);
	glCompileShader(regularFragmentShader);
	shaderLog(regularVertexShader);
	shaderLog(regularFragmentShader);
	glAttachShader(regularProgram, regularVertexShader);
	glAttachShader(regularProgram, regularFragmentShader);
	glLinkProgram(regularProgram);
	
	depthProgram = glCreateProgram();
	GLuint depthVertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint depthFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	char** depthVertexShaderSource = loadShaderSource("depth.vs.glsl");
	char** depthFragmentShaderSource = loadShaderSource("depth.fs.glsl");
	glShaderSource(depthVertexShader, 1, depthVertexShaderSource, NULL);
	glShaderSource(depthFragmentShader, 1, depthFragmentShaderSource, NULL);
	freeShaderSource(depthVertexShaderSource);
	freeShaderSource(depthFragmentShaderSource);
	glCompileShader(depthVertexShader);
	glCompileShader(depthFragmentShader);
	shaderLog(depthVertexShader);
	shaderLog(depthFragmentShader);
	glAttachShader(depthProgram, depthVertexShader);
	glAttachShader(depthProgram, depthFragmentShader);
	glLinkProgram(depthProgram);
	depth_um4mv = glGetUniformLocation(depthProgram, "um4mv");
	depth_um4p = glGetUniformLocation(depthProgram, "um4p");

	waterProgram = glCreateProgram();
	GLuint waterVertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint waterFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	char** waterVertexShaderSource = loadShaderSource("watervertex.vs.glsl");
	char** waterFragmentShaderSource = loadShaderSource("waterfragment.fs.glsl");
	glShaderSource(waterVertexShader, 1, waterVertexShaderSource, NULL);
	glShaderSource(waterFragmentShader, 1, waterFragmentShaderSource, NULL);
	freeShaderSource(waterVertexShaderSource);
	freeShaderSource(waterFragmentShaderSource);
	glCompileShader(waterVertexShader);
	glCompileShader(waterFragmentShader);
	shaderLog(waterVertexShader);
	shaderLog(waterFragmentShader);
	glAttachShader(waterProgram, waterVertexShader);
	glAttachShader(waterProgram, waterFragmentShader);
	glLinkProgram(waterProgram);
	water_um4mv = glGetUniformLocation(waterProgram, "um4mv");
	water_um4p = glGetUniformLocation(waterProgram, "um4p");
	water_tex = glGetUniformLocation(waterProgram, "tex");

	glGenVertexArrays(1, &window_vao);
	glBindVertexArray(window_vao);
	glGenBuffers(1, &window_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, window_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(window_positions), window_positions, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 4, 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 4, (const GLvoid*)(sizeof(GL_FLOAT) * 2));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glGenFramebuffers(1, &FBO);

	modle_pos = vec3(0.0f, 0.0f, 0.0f);
	cam_eye = vec3(0.0f, 150.0f, 0.0f);
	cam_up = vec3(0.0f, 1.0f, 0.0f);
	forward_vec = vec3(1.0f, 0.0f, 0.0f);
	light_eye = vec3(-200.0f, 200.0f, 100.0f);
	light_up = vec3(0.0f, 1.0f, 0.0f);
	light_center = vec3(0.0f, 0.0f, 0.0f);
	cam_center = cam_eye + forward_vec;
	
	model = translate(mat4(1.0f), modle_pos);

	light_proj_matrix = frustum(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1000.0f);
	light_view_matrix = lookAt(light_eye, light_center, light_up);

	glGenFramebuffers(1, &depth_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, depth_fbo);
	glGenTextures(1, &depth_tex);
	glBindTexture(GL_TEXTURE_2D, depth_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth_tex, 0);

	My_Reshape(600, 600);
}

void DrawWater() {
	glUseProgram(waterProgram);
	glUniformMatrix4fv(water_um4mv, 1, GL_FALSE, value_ptr(view * model));
	glUniformMatrix4fv(water_um4p, 1, GL_FALSE, value_ptr(proj_matrix));

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	//Enable shader layout location 0 and 1 for vertex and color   
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	//Create buffer and bind it to OpenGL
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);

	float seaDep = -10.0f;
	float sea_X = -100.0f;
	float sea_Z = -500.0f;

	float data[18] = {

		sea_X + 0.0f,   seaDep, sea_Z + 100.0f,
		sea_X + 0.0f,   seaDep, sea_Z + 0.0f,
		sea_X + 100.0f, seaDep, sea_Z + 0.0f,
		sea_X + 0.0f,   seaDep, sea_Z + 100.0f,
		sea_X + 100.0f, seaDep, sea_Z + 0.0f,
		sea_X + 100.0f, seaDep, sea_Z + 100.0f,

	};

	float texcoords[] = {
		0.0, 1.0,
		0.0, 0.0,
		1.0, 0.0,
		0.0, 1.0,
		1.0, 0.0,
		1.0, 1.0
	};
	GLuint waterTex;

	glGenTextures(1, &waterTex);
	glBindTexture(GL_TEXTURE_2D, waterTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WaterTextureData[WaterTimer/3].width, WaterTextureData[WaterTimer/3].height, 0, GL_RGBA, GL_UNSIGNED_BYTE, WaterTextureData[WaterTimer/3].data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glUniform1i(water_tex, 1);

	GLuint vertexbuffer, texbuffer;
	
	glGenBuffers(1, &texbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, texbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(texcoords), texcoords, GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, texbuffer);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);

	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	for (int i = 0; i < 60; i++) {
		for (int cnt = 0; cnt < 6; cnt++) {
			data[cnt * 3] += 6000.0f;
			data[2 + cnt * 3] += 100.0f;
		}
		for (int j = 0; j < 60; j++) {
			for (int cnt = 0; cnt < 6; cnt++) data[cnt*3] -= 100.0f;

			glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
	}

	WaterTimer++;
	if (WaterTimer == 300) WaterTimer = 0;
}

void DrawScene(Scene scene) {
	glUseProgram(program);
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * model));
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(proj_matrix));
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(shadowMap, 0);
	glBindTexture(GL_TEXTURE_2D, depth_tex);
	glActiveTexture(GL_TEXTURE1);
	glUniform1i(tex, 1);

	mat4 scale_bias_matrix = translate(mat4(), vec3(0.5f, 0.5f, 0.5f));
	scale_bias_matrix = scale(scale_bias_matrix, vec3(0.5f, 0.5f, 0.5f));
	mat4 shadow_sbpv_matrix = scale_bias_matrix*light_proj_matrix* light_view_matrix;
	mat4 shadow_matrix = shadow_sbpv_matrix * model;
	glUniformMatrix4fv(um4shadow, 1, GL_FALSE, value_ptr(shadow_matrix));
	glUniform3fv(uv3lightpos, 1, value_ptr(light_eye));

	for (unsigned int i = 0; i < scene.meshNum; ++i) {
		glBindVertexArray(scene.shapes[i].vao);
		int materialID = scene.shapes[i].materialID;
		glBindTexture(GL_TEXTURE_2D, scene.materials[materialID].diffuse_tex);
		glUniform1f(alpha, scene.materials[materialID].a);
		glUniform3fv(Ka, 1, value_ptr(scene.materials[materialID].Ka));
		glUniform3fv(Kd, 1, value_ptr(scene.materials[materialID].Kd));
		glUniform3fv(Ks, 1, value_ptr(scene.materials[materialID].Ks));
		glDrawElements(GL_TRIANGLES, scene.shapes[i].drawCount, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}
}

void DrawSky() {
	glUseProgram(skyProgram);
	glUniformMatrix3fv(skyeye, 1, GL_FALSE, value_ptr(cam_eye));
	glUniformMatrix4fv(skyvp, 1, GL_FALSE, value_ptr(proj_matrix * view));
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(skytex, 0);

	glBindVertexArray(skybox.vao);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex.diffuse_tex);
	glDrawElements(GL_TRIANGLE_STRIP, skybox.drawCount, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void DrawDepthMap(Scene scene) {
	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, depth_fbo);
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT); 
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(4.0f, 4.0f);
	glUseProgram(depthProgram);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUniformMatrix4fv(depth_um4mv, 1, GL_FALSE, value_ptr(light_view_matrix * model));
	glUniformMatrix4fv(depth_um4p, 1, GL_FALSE, value_ptr(light_proj_matrix));

	for (unsigned int i = 0; i < scene.meshNum; ++i) {
		glBindVertexArray(scene.shapes[i].vao);
		glDrawElements(GL_TRIANGLES, scene.shapes[i].drawCount, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}
}

void DrawReflection(Scene scene) {
	glViewport(0, 0, 1024, 1024);

}

void Regular() {
	glUseProgram(regularProgram);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(window_vao);
	glBindTexture(GL_TEXTURE_2D, FBODataTexture);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void My_Display()
{
	DrawDepthMap(scene2_1);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBO);
	glViewport(0, 0, window_size.x, window_size.y);
	glDrawBuffer(FBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	static const GLfloat white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	static const GLfloat one = 1.0f;
	glClearBufferfv(GL_COLOR, 0, white);
	glClearBufferfv(GL_DEPTH, 0, &one);

	cam_center = cam_eye + forward_vec;
	view = lookAt(cam_eye, cam_center, cam_up);
	
	DrawSky();
	DrawScene(scene2_1);
	DrawWater();

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glViewport(0, 0, window_size.x, window_size.y);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	Regular();

    glutSwapBuffers();
}

void My_Reshape(int width, int height)
{

	glViewport(0, 0, width, height);
	
	window_size = vec2(width, height);

	float viewportAspect = (float)width / (float)height;
	proj_matrix = perspective(radians(120.0f), viewportAspect, 0.1f, 10001.0f);
	view = lookAt(cam_eye, cam_center, cam_up);

	glDeleteRenderbuffers(1, &depthRBO);
	glDeleteTextures(1, &FBODataTexture);
	glGenRenderbuffers(1, &depthRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, depthRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, width, height);

	glGenTextures(1, &FBODataTexture);
	glBindTexture(GL_TEXTURE_2D, FBODataTexture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBO);
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRBO);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBODataTexture, 0);
}

void My_Timer(int val)
{
	timer_cnt++;
	glutPostRedisplay();
	if (timer_enabled)
	{
		glutTimerFunc(timer_speed, My_Timer, val);
	}
}

bool firstMouse = true;
void My_Mouse(int button, int state, int x, int y)
{
	if(state == GLUT_DOWN)
	{
		switch (button) {
			case GLUT_LEFT_BUTTON:
				break;
			case GLUT_RIGHT_BUTTON:
				break;
			default:
				break;
		}
		printf("Mouse %d is pressed at (%d, %d)\n", button, x, y);
	}
	else if(state == GLUT_UP)
	{
		switch (button) {
		case GLUT_LEFT_BUTTON:
			firstMouse = true;
			break;
		case GLUT_RIGHT_BUTTON:
			break;
		default:
			break;
		}
		printf("Mouse %d is released at (%d, %d)\n", button, x, y);
	}
}


int lastX;
int lastY;

float ya = 0.0f;
float pit = 0.0f;

void My_MouseMove(int x, int y) {

	if (firstMouse)
	{
		lastX = x;
		lastY = y;
		firstMouse = false;
	}

	GLfloat xoffset = x - lastX;
	GLfloat yoffset = lastY - y; // Reversed since y-coordinates go from bottom to left
	lastX = x;
	lastY = y;

	GLfloat sensitivity = 0.2;

	xoffset *= sensitivity;
	yoffset *= sensitivity;

	ya += xoffset;
	pit += yoffset;

	// Make sure that when pitch is out of bounds, screen doesn't get flipped
	if (pit > 89.0f)
		pit = 89.0f;
	if (pit < -89.0f)
		pit = -89.0f;

	vec3 front;
	front.x = cos(glm::radians(ya)) * cos(glm::radians(pit));
	front.y = sin(glm::radians(pit));
	front.z = sin(glm::radians(ya)) * cos(glm::radians(pit));
	forward_vec = normalize(front);
	
}

void My_Keyboard(unsigned char key, int x, int y)
{
	vec3 turn_vector;

	switch (key) {
	case 'w':
		//moving forward:
		cam_eye += forward_vec * (movementSpeed * (float)timer_speed / 1000);
		break;
	case 'a':
		//moving left:
		turn_vector = cross(cam_up, forward_vec);
		cam_eye += turn_vector * (movementSpeed * (float)timer_speed / 1000);
		break;
	case 's':
		//moving backward:
		cam_eye -= forward_vec * (movementSpeed * (float)timer_speed / 1000);
		break;
	case 'd':
		//moving right:
		turn_vector = cross(forward_vec, cam_up);
		cam_eye += turn_vector * (movementSpeed * (float)timer_speed / 1000);
		break;
	case 'z':
		//moving up:
		cam_eye += cam_up * (movementSpeed * (float)timer_speed / 1000);
		break;
	case 'x':
		//moving down:
		cam_eye -= cam_up * (movementSpeed * (float)timer_speed / 1000);
		break;
	default:
		break;
	}
	printf("cam_pos:(%f, %f, %f)\n", cam_eye.x, cam_eye.y, cam_eye.z);
}

void My_SpecialKeys(int key, int x, int y)
{
	switch(key)
	{
	case GLUT_KEY_F1:
		printf("F1 is pressed at (%d, %d)\n", x, y);
		break;
	case GLUT_KEY_PAGE_UP:
		printf("Page up is pressed at (%d, %d)\n", x, y);
		break;
	case GLUT_KEY_LEFT:
		printf("Left arrow is pressed at (%d, %d)\n", x, y);
		break;
	case GLUT_KEY_RIGHT:
		printf("Right arrow is pressed at (%d, %d)\n", x, y);
		break;
	default:
		printf("Other special key is pressed at (%d, %d)\n", x, y);
		break;
	}
}

void My_Menu(int id)
{
	switch(id)
	{
	case MENU_TIMER_START:
		if(!timer_enabled)
		{
			timer_enabled = true;
			glutTimerFunc(timer_speed, My_Timer, 0);
		}
		break;
	case MENU_TIMER_STOP:
		timer_enabled = false;
		break;
	case MENU_EXIT:
		exit(0);
		break;
	default:
		break;
	}
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
	glutInitWindowSize(600, 600);
	glutCreateWindow("Team5_FinalProject"); // You cannot use OpenGL functions before this line; The OpenGL context must be created first by glutCreateWindow()!
#ifdef _MSC_VER
	glewInit();
#endif
    glPrintContextInfo();
	My_Init();

	// Create a menu and bind it to mouse right button.
	int menu_main = glutCreateMenu(My_Menu);
	int menu_timer = glutCreateMenu(My_Menu);

	glutSetMenu(menu_main);
	glutAddSubMenu("Timer", menu_timer);
	glutAddMenuEntry("Exit", MENU_EXIT);

	glutSetMenu(menu_timer);
	glutAddMenuEntry("Start", MENU_TIMER_START);
	glutAddMenuEntry("Stop", MENU_TIMER_STOP);

	glutSetMenu(menu_main);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	// Register GLUT callback functions.
	glutDisplayFunc(My_Display);
	glutReshapeFunc(My_Reshape);
	glutMouseFunc(My_Mouse);
	glutMotionFunc(My_MouseMove);
	glutKeyboardFunc(My_Keyboard);
	glutSpecialFunc(My_SpecialKeys);
	glutTimerFunc(timer_speed, My_Timer, 0); 

	// Enter main event loop.
	glutMainLoop();

	return 0;
}
