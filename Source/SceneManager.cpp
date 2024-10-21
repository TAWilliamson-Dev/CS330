///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float xRotationDegrees,
	float yRotationDegrees,
	float zRotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(xRotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(yRotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(zRotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

void SceneManager::SetupSceneLights() {

	m_pShaderManager->setBoolValue("bUseLighting", true);

	m_pShaderManager->setVec3Value("lightSources[0].position", -9.0f, 7.0f, 4.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.5f, 0.5f, 0.5f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 2.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.3f);

	m_pShaderManager->setVec3Value("lightSources[1].position", 9.0f, 25.0f, -2.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 1.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.1f);
}


/*
   Load the selected textures into memory for use in the scene.
*/
void SceneManager::LoadSceneTextures() {
	bool bReturn;

	bReturn = CreateGLTexture("./textures/wax.jpg", "wax");

	bReturn = CreateGLTexture("./textures/wood_base.jpg", "wood");

	bReturn = CreateGLTexture("./textures/wood_worn.jpg", "wood_worn");

	bReturn = CreateGLTexture("./textures/placemat.jpg", "placemat");

	bReturn = CreateGLTexture("./textures/notebook_front.png", "d20");

	bReturn = CreateGLTexture("./textures/black_leather.png", "notebook");

	bReturn = CreateGLTexture("./textures/marble.jpg", "marble");

	bReturn = CreateGLTexture("./textures/metal.jpeg", "metal");

	bReturn = CreateGLTexture("./textures/glass.jpg", "glass");

	bReturn = CreateGLTexture("./textures/notebook_pages.jpg", "pages");
	
	BindGLTextures();
}

/*
  Define materials used in the scene.
*/
void SceneManager::DefineObjectMaterials() {
	
	OBJECT_MATERIAL waxMaterial;
	waxMaterial.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f);
	waxMaterial.ambientStrength = 0.5f;
	waxMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	waxMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	waxMaterial.shininess = 1.0f;
	waxMaterial.tag = "wax";

	m_objectMaterials.push_back(waxMaterial);

	OBJECT_MATERIAL placematMaterial;
	placematMaterial.ambientColor = glm::vec3(0.5f, 0.5f, 0.5f);
	placematMaterial.ambientStrength = 0.3f;
	placematMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	placematMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	placematMaterial.shininess = 10.0f;
	placematMaterial.tag = "placemat";

	m_objectMaterials.push_back(placematMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.5f, 0.5f, 0.5f);
	woodMaterial.ambientStrength = 0.3f;
	woodMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	woodMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	woodMaterial.shininess = 4.0f;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL woodMaterial_Gray;
	woodMaterial_Gray.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	woodMaterial_Gray.ambientStrength = 0.6f;
	woodMaterial_Gray.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	woodMaterial_Gray.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	woodMaterial_Gray.shininess = 4.0f;
	woodMaterial_Gray.tag = "wood_gray";

	m_objectMaterials.push_back(woodMaterial_Gray);

	OBJECT_MATERIAL woodMaterial_Black;
	woodMaterial_Black.ambientColor = glm::vec3(0.08f, 0.08f, 0.08f);
	woodMaterial_Black.ambientStrength = 0.4f;
	woodMaterial_Black.diffuseColor = glm::vec3(0.08f, 0.08f, 0.08f);
	woodMaterial_Black.specularColor = glm::vec3(0.16f, 0.16f, 0.16f);
	woodMaterial_Black.shininess = 3.0f;
	woodMaterial_Black.tag = "wood_black";

	m_objectMaterials.push_back(woodMaterial_Black);

	OBJECT_MATERIAL woodMaterial_Black_PencilCap;
	woodMaterial_Black_PencilCap.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f);
	woodMaterial_Black_PencilCap.ambientStrength = 0.6f;
	woodMaterial_Black_PencilCap.diffuseColor = glm::vec3(0.05f, 0.05f, 0.05f);
	woodMaterial_Black_PencilCap.specularColor = glm::vec3(0.09f, 0.09f, 0.09f);
	woodMaterial_Black_PencilCap.shininess = 3.0f;
	woodMaterial_Black_PencilCap.tag = "wood_black_pencilcap";

	m_objectMaterials.push_back(woodMaterial_Black_PencilCap);

	OBJECT_MATERIAL woodTableMaterial;
	woodTableMaterial.ambientColor = glm::vec3(0.5f, 0.5f, 0.5f);
	woodTableMaterial.ambientStrength = 0.7f;
	woodTableMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	woodTableMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	woodTableMaterial.shininess = 3.0f;
	woodTableMaterial.tag = "woodtable";

	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL notebookCoverMaterial;
	notebookCoverMaterial.ambientColor = glm::vec3(0.5f, 0.5f, 0.5f);
	notebookCoverMaterial.ambientStrength = 0.3f;
	notebookCoverMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	notebookCoverMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	notebookCoverMaterial.shininess = 3.0f;
	notebookCoverMaterial.tag = "notebookfront";

	m_objectMaterials.push_back(notebookCoverMaterial);

	OBJECT_MATERIAL goldMaterial;
	goldMaterial.ambientColor = glm::vec3(0.31f, 0.251f, 0.029f);
	goldMaterial.ambientStrength = 0.3f;
	goldMaterial.diffuseColor = glm::vec3(0.31f, 0.251f, 0.029f);
	goldMaterial.specularColor = glm::vec3(0.4f, 0.39f, 0.39f);
	goldMaterial.shininess = 11.0f;
	goldMaterial.tag = "metal_gold";

	m_objectMaterials.push_back(goldMaterial);

	OBJECT_MATERIAL marbleMaterial_blue;
	marbleMaterial_blue.ambientColor = glm::vec3(0.01f, 0.01f, 0.5f);
	marbleMaterial_blue.ambientStrength = 0.5f;
	marbleMaterial_blue.diffuseColor = glm::vec3(0.05f, 0.05f, 0.5f);
	marbleMaterial_blue.specularColor = glm::vec3(0.24f, 0.24f, 0.7f);
	marbleMaterial_blue.shininess = 13.0f;
	marbleMaterial_blue.tag = "marble_blue";

	m_objectMaterials.push_back(marbleMaterial_blue);

	OBJECT_MATERIAL marbleMaterial_green;
	marbleMaterial_green.ambientColor = glm::vec3(0.01f, 0.5f, 0.01f);
	marbleMaterial_green.ambientStrength = 0.5f;
	marbleMaterial_green.diffuseColor = glm::vec3(0.05f, 0.5f, 0.05f);
	marbleMaterial_green.specularColor = glm::vec3(0.05f, 0.9f, 0.05f);
	marbleMaterial_green.shininess = 13.0f;
	marbleMaterial_green.tag = "marble_green";

	m_objectMaterials.push_back(marbleMaterial_green);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.05f, 0.03f, 0.05f);
	glassMaterial.ambientStrength = 0.5f;
	glassMaterial.diffuseColor = glm::vec3(0.05f, 0.02f, 0.02f);
	glassMaterial.specularColor = glm::vec3(0.25f, 0.15f, 0.15f);
	glassMaterial.shininess = 12.0f;
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);
	
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	DefineObjectMaterials();

	LoadSceneTextures();

	SetupSceneLights();

	m_basicMeshes->LoadPlaneMesh();

	m_basicMeshes->LoadCylinderMesh(); 
	m_basicMeshes->LoadConeMesh();     
	m_basicMeshes->LoadSphereMesh();   
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadTorusMesh(0.05f);
	m_basicMeshes->LoadPyramid4Mesh();

}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

	RenderTable();
	RenderPencil();
	RenderNotebook();
	RenderCandle();
	RenderDEight();
	RenderDSix();
	RenderCandleLid();
}

/* 
 * Sets the transforms of the 3 cylinders and a half sphere 
 * that make up the pencil object in the scene. 
 */
void SceneManager::RenderPencil() {
	
	glm::vec3 scaleXYZ;
	float xRotationDegrees = 0.0f;
	float yRotationDegrees = 0.0f;
	float zRotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
	
	/*************************** PENCIL ***************************/

	/* Pencil - Cylinder */

	scaleXYZ = glm::vec3(0.125f, 3.8f, 0.125f);

	xRotationDegrees = 0.0f;
	yRotationDegrees = 43.0f;
	zRotationDegrees = 90.0f;

	positionXYZ = glm::vec3(-0.04477f, 0.85f, 2.63465f);

	SetTransformations(
		scaleXYZ,
		xRotationDegrees,
		yRotationDegrees,
		zRotationDegrees,
		positionXYZ);

	SetShaderTexture("wood");
	SetTextureUVScale(0.5, 3.0);
	SetShaderMaterial("wood");

	m_basicMeshes->DrawCylinderMesh(false, false, true);

	/* Pencil - Cone */

	scaleXYZ = glm::vec3(0.125f, 0.5f, 0.125f);

	xRotationDegrees = 0.0f;
	yRotationDegrees = 43.0f;
	zRotationDegrees = 90.0f;

	positionXYZ = glm::vec3(-2.82505f, 0.85f, 5.22505f);

	SetTransformations(
		scaleXYZ,
		xRotationDegrees,
		yRotationDegrees,
		zRotationDegrees,
		positionXYZ);

	SetShaderTexture("wood_worn");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	m_basicMeshes->DrawConeMesh(false);

	/* Pencil - Mid Cylinder */

	scaleXYZ = glm::vec3(0.125f, 0.2f, 0.125f);

	xRotationDegrees = 0.0f;
	yRotationDegrees = 43.0f;
	zRotationDegrees = 90.0f;

	positionXYZ = glm::vec3(0.1026f, 0.85f, 2.4995f);

	SetTransformations(
		scaleXYZ,
		xRotationDegrees,
		yRotationDegrees,
		zRotationDegrees,
		positionXYZ);

	SetShaderTexture("wood");
	SetTextureUVScale(3.0, 3.0);
	SetShaderMaterial("wood_gray");


	m_basicMeshes->DrawCylinderMesh(false, false, true);

	/* Pencil - Top Cylinder */

	scaleXYZ = glm::vec3(0.125f, 0.5f, 0.125f);

	xRotationDegrees = 0.0f;
	yRotationDegrees = 43.0f;
	zRotationDegrees = 90.0f;

	positionXYZ = glm::vec3(0.4695f, 0.85f, 2.156f);

	SetTransformations(
		scaleXYZ,
		xRotationDegrees,
		yRotationDegrees,
		zRotationDegrees,
		positionXYZ);

	SetShaderTexture("wood");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood_black");


	m_basicMeshes->DrawCylinderMesh(false, false, true);

	/* Pencil - Half Sphere Cap */

	scaleXYZ = glm::vec3(0.125f, 0.1f, 0.125f);

	xRotationDegrees = 180.0f;
	yRotationDegrees = 137.0f;
	zRotationDegrees = 90.0f;

	positionXYZ = glm::vec3(0.4695f, 0.85f, 2.156f);

	SetTransformations(
		scaleXYZ,
		xRotationDegrees,
		yRotationDegrees,
		zRotationDegrees,
		positionXYZ);

	SetShaderTexture("wood");
	SetTextureUVScale(3.0, 3.0);
	SetShaderMaterial("wood_black_pencilcap");

	m_basicMeshes->DrawHalfSphereMesh();
}

/*
*  Sets the transform of the boxes and plane that make up the notebook object
*  in the scene.
*/
void SceneManager::RenderNotebook() {

	glm::vec3 scaleXYZ;
	float xRotationDegrees = 0.0f;
	float yRotationDegrees = 0.0f;
	float zRotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
	/*************************** NOTEBOOK ***************************/

	/* Notebook - Pages */

	scaleXYZ = glm::vec3(5.35f, 0.40f, 4.2f);

	xRotationDegrees = 0.0f;
	yRotationDegrees = 80.0f;
	zRotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-1.5f, 0.5f, 3.3f);

	SetTransformations(
		scaleXYZ,
		xRotationDegrees,
		yRotationDegrees,
		zRotationDegrees,
		positionXYZ);


	SetShaderTexture("pages");
	SetTextureUVScale(0.25, 0.25);
	SetShaderMaterial("notebookfront");

	m_basicMeshes->DrawBoxMesh();

	/* Notebook - Top Cover */

	scaleXYZ = glm::vec3(4.25f, 0.05f, 5.5f);

	xRotationDegrees = 0.0f;
	yRotationDegrees = -10.0f;
	zRotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-1.5f, 0.7f, 3.3f);

	SetTransformations(
		scaleXYZ,
		xRotationDegrees,
		yRotationDegrees,
		zRotationDegrees,
		positionXYZ);

	SetShaderTexture("notebook");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("notebookfront");

	m_basicMeshes->DrawBoxMesh();

	/* Notebook - Bottom Cover */

	scaleXYZ = glm::vec3(5.5f, 0.05f, 4.25f);

	xRotationDegrees = 0.0f;
	yRotationDegrees = 80.0f;
	zRotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-1.5f, 0.3f, 3.3f);

	SetTransformations(
		scaleXYZ,
		xRotationDegrees,
		yRotationDegrees,
		zRotationDegrees,
		positionXYZ);

	SetShaderTexture("notebook");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("notebookfront");

	m_basicMeshes->DrawBoxMesh();

	/* Notebook - Cover */

	scaleXYZ = glm::vec3(2.1f, 1.0f, 2.65f);

	xRotationDegrees = 0.0f;
	yRotationDegrees = -10.0f;
	zRotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-1.5f, 0.75f, 3.3f);

	SetTransformations(
		scaleXYZ,
		xRotationDegrees,
		yRotationDegrees,
		zRotationDegrees,
		positionXYZ);

	SetShaderTexture("d20");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("notebookfront");

	m_basicMeshes->DrawPlaneMesh();

	/* Notebook - Spine */

	scaleXYZ = glm::vec3(5.5f, 0.45f, 0.095f);

	xRotationDegrees = 0.0f;
	yRotationDegrees = 80.0f;
	zRotationDegrees = 0.0f;

	//	positionXYZ = glm::vec3(-3.59f, 0.5f, 2.9f);
	positionXYZ = glm::vec3(-3.593f, 0.499f, 2.93f);

	SetTransformations(
		scaleXYZ,
		xRotationDegrees,
		yRotationDegrees,
		zRotationDegrees,
		positionXYZ);

	//SetShaderColor(0.15f, 0.15f, 0.15f, 1.0f);
	SetShaderTexture("notebook");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("notebookfront");

	m_basicMeshes->DrawBoxMesh();

}

/*
*  Sets the transforms of the cylinder and plane that represent the table and placemat in the scene.
*/
void SceneManager::RenderTable() {

	glm::vec3 scaleXYZ;
	float xRotationDegrees = 0.0f;
	float yRotationDegrees = 0.0f;
	float zRotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(10.0f, 1.0f, 5.0f);

	// set the XYZ rotation for the mesh
	xRotationDegrees = 0.0f;
	yRotationDegrees = 0.0f;
	zRotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.8f, 0.26f, 2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		xRotationDegrees,
		yRotationDegrees,
		zRotationDegrees,
		positionXYZ);

	SetShaderTexture("placemat");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("placemat");


	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	/*************************** TABLE ***************************/

		// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 0.25f, 20.0f);

	// set the XYZ rotation for the mesh
	xRotationDegrees = 0.0f;
	yRotationDegrees = 0.0f;
	zRotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		xRotationDegrees,
		yRotationDegrees,
		zRotationDegrees,
		positionXYZ);

	SetShaderTexture("wood");
	SetTextureUVScale(3.0, 3.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
}

/*
*  Sets the transforms of the cylinders and torus that make up the candle object.
*  Currently has a shading mismatch between the torus and cylinder that make up the jar, 
* 
*/
void SceneManager::RenderCandle() {

	glm::vec3 scaleXYZ;
	float xRotationDegrees = 0.0f;
	float yRotationDegrees = 0.0f;
	float zRotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/* Candle - Candle */

	scaleXYZ = glm::vec3(1.2f, 2.6f, 1.2f);

	xRotationDegrees = 180.0f;
	yRotationDegrees = 0.0f;
	zRotationDegrees = 0.0f;

	positionXYZ = glm::vec3(2.493f, 3.2f, 1.38f);

	SetTransformations(
		scaleXYZ,
		xRotationDegrees,
		yRotationDegrees,
		zRotationDegrees,
		positionXYZ);

	SetShaderTexture("wax");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wax");

	m_basicMeshes->DrawCylinderMesh();

	/* Candle - Interior */

	scaleXYZ = glm::vec3(1.21f, 2.6f, 1.21f);

	xRotationDegrees = 180.0f;
	yRotationDegrees = 0.0f;
	zRotationDegrees = 0.0f;

	positionXYZ = glm::vec3(2.493f, 3.4f, 1.38f);

	SetTransformations(
		scaleXYZ,
		xRotationDegrees,
		yRotationDegrees,
		zRotationDegrees,
		positionXYZ);

	SetShaderTexture("glass");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("glass");

	m_basicMeshes->DrawCylinderMesh(false, false, true);

	/* Candle - Jar Lip */

	scaleXYZ = glm::vec3(1.24f, 1.24f, 0.5f);

	xRotationDegrees = 90.0f;
	yRotationDegrees = 0.0f;
	zRotationDegrees = 190.0f;

	positionXYZ = glm::vec3(2.493f, 3.41f, 1.38f);

	SetTransformations(
		scaleXYZ,
		xRotationDegrees,
		yRotationDegrees,
		zRotationDegrees,
		positionXYZ);

	SetShaderTexture("glass");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("glass");

	m_basicMeshes->DrawTorusMesh();

	/* Candle - Jar */

	scaleXYZ = glm::vec3(1.3f, 3.1f, 1.3f);

	xRotationDegrees = 180.0f;
	yRotationDegrees = 0.0f;
	zRotationDegrees = 0.0f;

	positionXYZ = glm::vec3(2.493f, 3.4, 1.38f);

	SetTransformations(
		scaleXYZ,
		xRotationDegrees,
		yRotationDegrees,
		zRotationDegrees,
		positionXYZ);

	SetShaderTexture("glass");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("glass");

	m_basicMeshes->DrawCylinderMesh(true, false, true);

	/* Candle - Wick */

	scaleXYZ = glm::vec3(0.05f, 0.6f, 0.05f);

	xRotationDegrees = -25.0f;
	yRotationDegrees = 0.0f;
	zRotationDegrees = -20.0f;

	positionXYZ = glm::vec3(2.493f, 3.15, 1.38f);

	SetTransformations(
		scaleXYZ,
		xRotationDegrees,
		yRotationDegrees,
		zRotationDegrees,
		positionXYZ);

	SetShaderColor(0.83f, 0.79f, 0.705f, 1.0f);
	SetShaderMaterial("wax");

	m_basicMeshes->DrawCylinderMesh();


}

/*
* Sets the transforms of the two 4 sided pyramids that comprise the D8 in the scene.
*/
void SceneManager::RenderDEight() {

	glm::vec3 scaleXYZ;
	float xRotationDegrees = 0.0f;
	float yRotationDegrees = 0.0f;
	float zRotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/* D8 - Pyramid 1 */

	scaleXYZ = glm::vec3(0.35f, 0.2f, 0.35f);

	xRotationDegrees = -50.0f;
	yRotationDegrees = 0.0f;
	zRotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-2.593f, 0.954f, 2.02725);

	SetTransformations(
		scaleXYZ,
		xRotationDegrees,
		yRotationDegrees,
		zRotationDegrees,
		positionXYZ);

	//SetShaderColor(0.0f, 0.25, 0.65f, 1.0f);
	SetShaderTexture("marble");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("marble_blue");

	m_basicMeshes->DrawPyramid4Mesh();

	/* D8 - Pyramid 2 */

	scaleXYZ = glm::vec3(0.35f, 0.2f, 0.35f);

	xRotationDegrees = 130.0f;
	yRotationDegrees = 0.0f;
	zRotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-2.593f, 0.825f, 2.18f);

	SetTransformations(
		scaleXYZ,
		xRotationDegrees,
		yRotationDegrees,
		zRotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawPyramid4Mesh();
}

/*
*  Sets transform of the box that represents the D6 in the scene.
*/
void SceneManager::RenderDSix() {

	glm::vec3 scaleXYZ;
	float xRotationDegrees = 0.0f;
	float yRotationDegrees = 0.0f;
	float zRotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/* D6 */

	scaleXYZ = glm::vec3(0.25f, 0.25f, 0.25f);

	xRotationDegrees = 0.0f;
	yRotationDegrees = 15.0f;
	zRotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-1.493f, 0.88f, 1.38f);

	SetTransformations(
		scaleXYZ,
		xRotationDegrees,
		yRotationDegrees,
		zRotationDegrees,
		positionXYZ);

	//SetShaderColor(0.15f, 0.55f, 0.35f, 1.0f);
	SetShaderTexture("marble");
	SetTextureUVScale(3.1, 2.9);
	SetShaderMaterial("marble_green");

	m_basicMeshes->DrawBoxMesh();
}

/*
* Sets the transforms of the two cylinders that represent the candle jars lid in the scene.
*/
void SceneManager::RenderCandleLid() {

	glm::vec3 scaleXYZ;
	float xRotationDegrees = 0.0f;
	float yRotationDegrees = 0.0f;
	float zRotationDegrees = 0.0f;
	glm::vec3 positionXYZ;


	/* Jar Lid */

	scaleXYZ = glm::vec3(1.3f, 0.1f, 1.3f);


	xRotationDegrees = 180.0f;
	yRotationDegrees = 65.0f;
	zRotationDegrees = 35.0f;

	positionXYZ = glm::vec3(4.15f, 1.15, 3.18f);

	SetTransformations(
		scaleXYZ,
		xRotationDegrees,
		yRotationDegrees,
		zRotationDegrees,
		positionXYZ);

	SetShaderTexture("metal");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal_gold");

	m_basicMeshes->DrawCylinderMesh();


	/* Lid - Seal Inner */

	scaleXYZ = glm::vec3(1.2f, 0.2f, 1.2f);


	xRotationDegrees = 180.0f;
	yRotationDegrees = 65.0f;
	zRotationDegrees = 35.0f;

	positionXYZ = glm::vec3(4.15f, 1.15, 3.18f);

	SetTransformations(
		scaleXYZ,
		xRotationDegrees,
		yRotationDegrees,
		zRotationDegrees,
		positionXYZ);

	SetShaderTexture("metal");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal_gold");

	m_basicMeshes->DrawCylinderMesh(false, false, true);

	/* Lid - Seal Outer */

	scaleXYZ = glm::vec3(1.2f, 0.2f, 1.2f);


	xRotationDegrees = 180.0f;
	yRotationDegrees = 65.0f;
	zRotationDegrees = 35.0f;

	positionXYZ = glm::vec3(4.15f, 1.15, 3.18f);

	SetTransformations(
		scaleXYZ,
		xRotationDegrees,
		yRotationDegrees,
		zRotationDegrees,
		positionXYZ);

	SetShaderTexture("metal");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal_gold");

	m_basicMeshes->DrawCylinderMesh(false, false, true);

}