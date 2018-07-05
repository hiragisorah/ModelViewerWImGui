#pragma once

#pragma warning(disable: 4996)

#include <filesystem>
#include <string>

namespace sys = std::tr2::sys;

class PathManager
{
public:
	PathManager(void)
		: current_path_(".")
	{
		this->UpdateDirectory();
	}

private:
	sys::path current_path_;
	std::vector<sys::path> folder_list_;
	std::vector<sys::path> file_list_;

public:
	const unsigned int FolderCnt(void)
	{
		return folder_list_.size();
	}
	const unsigned int FileCnt(void)
	{
		return file_list_.size();
	}
	void BackDirectory(void)
	{
		this->current_path_ = this->current_path_.parent_path();
		this->UpdateDirectory();
	}
	void SetPath(sys::path path)
	{
		this->current_path_ = path;
		this->UpdateDirectory();
	}
	std::vector<sys::path> & Folders(void)
	{
		return this->folder_list_;
	}
	std::vector<sys::path> & Files(void)
	{
		return this->file_list_;
	}

private:
	void UpdateDirectory(void)
	{
		this->folder_list_.clear();
		this->file_list_.clear();
		std::for_each(sys::directory_iterator(this->current_path_), sys::directory_iterator(),
			[&](const sys::path & p)
		{
			if (sys::is_directory(p))
			{
				this->folder_list_.emplace_back(p);
			}
			if (sys::is_regular_file(p))
			{
				this->file_list_.emplace_back(p);
			}
		});
	}
};