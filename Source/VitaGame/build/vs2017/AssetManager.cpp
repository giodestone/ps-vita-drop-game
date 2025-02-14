#include "AssetManager.h"

#include <system/debug_log.h>

#include <assets/png_loader.h>
#include <graphics/image_data.h>
#include <graphics/texture.h>

AssetManager::AssetManager(gef::Platform* platform, gef::AudioManager* am)
	: platform(platform),
	audioManager(am)
{
}


AssetManager::~AssetManager()
{
	//Delete scenes and meshes. 
	/* As meshes generated by the primitive builder and meshes from scenes are in the same array you could end up deleting meshes inside of the scene causing an error when exiting
	 * This aims to fix by keeping track what is and what isnt in a scene. This is calcualted on loadmesh and addmesh.
	 */
	for (auto isInScene = isMeshInScene.begin(); isInScene != isMeshInScene.end();)
	{
		if ((*isInScene).second) //if the mesh contained at the alias is in the scene
		{
			auto sceneAt = scenes.at((*isInScene).first); //get the scene at the value
			delete sceneAt; //free memory
			sceneAt = nullptr; //remove scene
			meshes.erase((*isInScene).first); //erase the mesh reference from the scenes
		}
		else //otherwise just remove mesh
		{
			auto meshAt = meshes.at((*isInScene).first); //get the mesh at the current key value
			delete meshAt; //free memory
			meshAt = nullptr; //deallocate
			meshes.erase((*isInScene).first); //remove the mesh from the array
		}

		isInScene = isMeshInScene.erase(isInScene); //move onto the next one
	}

	if (audioManager != nullptr) //make it work on windows
	{
		audioManager->UnloadAllSamples(); //unload sf
		audioManager->UnloadMusic(); //unload music
	}

	for (auto it = fonts.begin(); it != fonts.end();) //unload all fonts.
	{
		delete (*it).second;
		(*it).second = nullptr;
		it = fonts.erase(it);
	}

	for (auto it = textures.begin(); it != textures.end();) //unload all textures
	{
		delete (*it).second;
		(*it).second = nullptr;
		it = textures.erase(it);
	}
}

int AssetManager::LoadAudioSample(std::string path, std::string alias)
{
	if (audioManager == nullptr) //if audio manager doesnt exist.
		return -1;

	int id = audioManager->LoadSample(path.data(), *platform);
	audioSamples.insert(std::pair<std::string, int>(alias, id)); //associate with alias
	return id;
}

void AssetManager::LoadMusic(const char * path)
{
	if(audioManager == nullptr)
		return;

	auto loadSuccess = audioManager->LoadMusic(path, *platform);
	
	if (loadSuccess < 0)
		gef::DebugOut("Failed to load music: %s", path);
}

void AssetManager::LoadMesh(std::string path, std::string alias)
{
	auto scene = LoadSceneAssets(*platform, path.data());
	if (scene == nullptr) //if fails to load
	{
		gef::DebugOut("ERROR: Scene file %s failed to load\n", path.data());
		return;
	}
	scenes.insert(std::pair<std::string, gef::Scene*>(alias, scene)); //create scene
	meshes.insert(std::pair<std::string, gef::Mesh*>(alias, GetMeshFromSceneAssets(scene))); //now get a mesh and pass it to the map
	isMeshInScene.insert(std::pair<std::string, bool>(alias, true));
}

void AssetManager::LoadFont(std::string path, std::string alias)
{
	auto font = new gef::Font(*platform);
	font->Load(path.data());
	fonts.insert(std::pair<std::string, gef::Font*>(alias, font));
}

void AssetManager::LoadTexture(std::string path, std::string alias)
{
	gef::ImageData imgData;
	pngLoader.Load(path.data(), *platform, imgData);

	auto imgTexture = gef::Texture::Create(*platform, imgData);

	textures.insert(std::pair<std::string, gef::Texture*>(alias, imgTexture));
}

void AssetManager::AddMesh(gef::Mesh * mesh, std::string alias)
{
	meshes.insert(std::pair<std::string, gef::Mesh*>(alias, mesh));
	isMeshInScene.insert(std::pair<std::string, bool>(alias, false));
}

gef::Mesh * AssetManager::GetMesh(std::string alias)
{
	return meshes.at(alias); //this error is a lie
}

gef::Font * AssetManager::GetFont(std::string alias)
{
	return fonts.at(alias);
}

gef::Texture * AssetManager::GetTexture(std::string alias)
{
	return textures.at(alias);
}

int AssetManager::GetAudioSample(std::string alias)
{
	if (audioManager == nullptr) //if audio manager doesn't exist
		return -1;

	return audioSamples.at(alias); //as is this one
}

gef::Scene * AssetManager::LoadSceneAssets(gef::Platform & platform, const char * filename)
{
	gef::Scene* scene = new gef::Scene();

	if (scene->ReadSceneFromFile(platform, filename))
	{
		// if scene file loads successful
		// create material and mesh resources from the scene data
		scene->CreateMaterials(platform);
		scene->CreateMeshes(platform);
	}
	else
	{
		delete scene;
		scene = NULL;
	}

	return scene;
}

gef::Mesh * AssetManager::GetMeshFromSceneAssets(gef::Scene * scene)
{
	gef::Mesh* mesh = nullptr;

	// if the scene data contains at least one mesh
	// return the first mesh
	if (scene && scene->meshes.size() > 0)
		mesh = scene->meshes.front();

	return mesh;

}
