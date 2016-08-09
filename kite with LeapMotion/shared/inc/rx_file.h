/*! @file rx_file.h
	
	@brief �t�@�C������(boost�g�p)
 
	@author Makoto Fujisawa
	@date 2009-02
*/

#ifndef _RX_COMMON_FILE_H_
#define _RX_COMMON_FILE_H_


//-----------------------------------------------------------------------------
// �C���N���[�h�t�@�C��
//-----------------------------------------------------------------------------
#include <vector>
#include <string>

#include "boost/filesystem/path.hpp"
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/fstream.hpp"

#include "boost/bind.hpp"
#include "boost/function.hpp"


//-----------------------------------------------------------------------------
// MARK:�t�@�C���E�t�H���_����
//-----------------------------------------------------------------------------
namespace RXFile
{
	typedef boost::filesystem::path fspath;

	/*!
	 * �ċA�I�ɑS�t�@�C�������o��
	 * @param[in] fpath �t�H���_�p�X
	 * @param[out] paths ���������t�@�C���ꗗ
	 */
	static void Search(const fspath &dpath, std::vector<std::string> &paths)
	{
		// �J�����g�f�B���N�g���̃t�@�C���ꗗ
		boost::filesystem::directory_iterator end; 
		for(boost::filesystem::directory_iterator it(dpath); it != end; ++it){
			if(boost::filesystem::is_directory(*it)){
				Search(it->path(), paths);
			}
			else{
				paths.push_back(it->path().generic_string());	// native�t�H�[�}�b�g�̏ꍇ�͒P��string()
			}
		} 
	}
	static void Search(const std::string &dir, std::vector<std::string> &paths)
	{
		fspath dpath(dir);
		if(boost::filesystem::is_directory(dpath)){
			Search(dpath, paths);
		}
	}

	/*!
	 * �ċA�I�ɑS�t�@�C�������o��
	 * @param[in] fpath �t�H���_�p�X
	 * @param[out] paths ���������t�@�C���ꗗ
	 * @param[in] fpComp ��������
	 */
	static void Search(const fspath &dpath, 
					   std::vector<std::string> &paths, 
					   boost::function<bool (std::string)> fpComp)
	{
		// �J�����g�f�B���N�g���̃t�@�C���ꗗ
		boost::filesystem::directory_iterator end; 
		for(boost::filesystem::directory_iterator it(dpath); it!=end; ++it){
			if(boost::filesystem::is_directory(*it)){
				Search(it->path(), paths);
			}
			else{
				std::string fpath = it->path().generic_string();
				if(fpComp(fpath)){
					paths.push_back(fpath);
				}
			}
		}
	}
	static void Search(const std::string &dir, 
		               std::vector<std::string> &paths, 
					   boost::function<bool (std::string)> fpComp)
	{
		fspath dpath(dir);
		if(boost::filesystem::is_directory(dpath)){
			Search(dpath, paths, fpComp);
		}
	}

	/*!
	 * �t�@�C������r�֐�(�g���q)
	 * @param[in] fn ��r�������t�@�C����
	 * @param[in] ext �g���q
	 * @return fn�̊g���q��ext�Ɠ����Ȃ�true
	 */
	inline bool SearchCompExt(std::string fn, std::string ext)
	{
		return (fn.find(ext, 0) != std::string::npos);
	}

	/*!
	 * �t�@�C������r�֐�(�����g���q)
	 * @param[in] fn ��r�������t�@�C����
	 * @param[in] ext �g���q
	 * @return fn�̊g���q��ext�Ɠ����Ȃ�true
	 */
	inline bool SearchCompExts(std::string fn, vector<std::string> exts)
	{
		vector<std::string>::iterator i;
		for(i = exts.begin(); i != exts.end(); ++i){
			if(fn.find(*i, 0) != std::string::npos) break;
		}

		return (i != exts.end());
	}

	/*!
	 * �ċA�I�ɑS�t�@�C�������o��(�q�t�H���_�̊K�w�ő�l�w��)
	 * @param[in] fpath �t�H���_�p�X
	 * @param[out] paths ���������t�@�C���ꗗ
	 * @param[inout] d ���݂̊K�w��
	 * @param[in] n �ő�K�w��
	 */
	static void Search(const fspath &dpath, std::vector<std::string> &paths, int d, const int n)
	{
		// �J�����g�f�B���N�g���̃t�@�C���ꗗ
		boost::filesystem::directory_iterator end; 
		for(boost::filesystem::directory_iterator it(dpath); it!=end; ++it){
			if(boost::filesystem::is_directory(*it) && (n == -1 || d < n)){
				Search(it->path(), paths, d+1, n);
			}
			else{
				paths.push_back(it->path().generic_string());
			}
		}
	}

	/*!
	 * �ċA�I�ɑS�t�@�C�������o��
	 * @param[in] fpath �t�H���_�p�X
	 * @param[out] paths ���������t�@�C���ꗗ
	 * @param[inout] d ���݂̊K�w��
	 * @param[in] n �ő�K�w��
	 * @param[in] fpComp ��������
	 */
	static void Search(const fspath &dpath, std::vector<std::string> &paths, int d, const int n, 
					   boost::function<bool (std::string)> fpComp)
	{
		// �J�����g�f�B���N�g���̃t�@�C���ꗗ
		boost::filesystem::directory_iterator end; 
		for(boost::filesystem::directory_iterator it(dpath); it!=end; ++it){
			if(boost::filesystem::is_directory(*it) && (n == -1 || d < n)){
				Search(it->path(), paths, d+1, n, fpComp);
			}
			else{
				std::string fpath = it->path().generic_string();
				if(fpComp(fpath)){
					paths.push_back(fpath);
				}
			}
		}
	}

	static void Search(const std::string &dir, std::vector<std::string> &paths, const int n)
	{
		fspath dir_path(dir);
		if(boost::filesystem::is_directory(dir_path)){
			int d = 0;
			Search(dir_path, paths, d, n);
		}
	}

	static void Search(const std::string &dir, std::vector<std::string> &paths, std::string ext, const int n)
	{
		fspath dir_path(dir);
		if(boost::filesystem::is_directory(dir_path)){
			boost::function<bool (std::string)> fpComp;
			fpComp = boost::bind(SearchCompExt, _1, ext);

			int d = 0;
			Search(dir_path, paths, d, n, fpComp);
		}
	}

	static void Search(const std::string &dir, std::vector<std::string> &paths, 
					   boost::function<bool (std::string)> fpComp, const int n)
	{
		fspath dir_path(dir);
		if(boost::filesystem::is_directory(dir_path)){
			int d = 0;
			Search(dir_path, paths, d, n, fpComp);
		}
	}

	static void Search(const std::string &dir, std::vector<std::string> &paths, vector<std::string> exts, const int n)
	{
		fspath dir_path(dir);
		if(boost::filesystem::is_directory(dir_path)){
			boost::function<bool (std::string)> fpComp;
			fpComp = boost::bind(SearchCompExts, _1, exts);

			int d = 0;
			Search(dir_path, paths, d, n, fpComp);
		}
	}


	/*!
	 * �t�@�C��/�t�H���_�̍쐬�����m�F
	 * @param[in] path �t�@�C���p�X
	 * @return �쐬�����C���݂��Ȃ����-1��Ԃ�
	 */
	inline long WriteTime(const std::string &path)
	{
		fspath file_path(path);
		if(boost::filesystem::exists(file_path)){
			long t = (long)boost::filesystem::last_write_time(file_path);
			return t;
		}
		else{
			return -1;
		}
	}

	/*!
	 * �t�@�C���̃T�C�Y�m�F
	 * @param[in] path �t�@�C���p�X
	 * @return �T�C�Y(�o�C�g)�C���݂��Ȃ����-1��Ԃ�
	 */
	inline long Size(const std::string &path)
	{
		fspath file_path(path);
		if(boost::filesystem::exists(file_path)){
			long s = (long)boost::filesystem::file_size(file_path);
			return s;
		}
		else{
			return -1;
		}
	}

	/*!
	 * �t�@�C��/�t�H���_�폜(�t�@�C���Ƌ�t�H���_�̂�)
	 * @param[in] path �t�@�C���E�t�H���_�p�X
	 */
	inline void Remove(const std::string &path)
	{
		fspath file_path(path);
		boost::filesystem::remove(file_path);
	}

	/*!
	 * �t�@�C��/�t�H���_�폜(���ׂ�)
	 * @param[in] path �t�@�C���E�t�H���_�p�X
	 * @return �폜�t�@�C����
	 */
	inline unsigned long RemoveAll(const std::string &path)
	{
		fspath file_path(path);
		return (unsigned long)boost::filesystem::remove_all(file_path);
	}

	/*!
	 * �t�@�C��/�t�H���_�̑��݊m�F
	 * @param[in] path �t�@�C���E�t�H���_�p�X
	 * @return ���݂��邩�ǂ���
	 */
	inline bool Exist(const std::string &path)
	{
		fspath file_path(path);
		return boost::filesystem::exists(file_path);
		//FILE *fp;
		//if( (fp = fopen(path.c_str(), "r")) == NULL ){
		//	return false;
		//}
		//fclose(fp);
		//return true;
	}

	/*!
	 * ���l�[���^�ړ�
	 *  - �������[�g�̂���t�@�C���V�X�e��(Windows�܂�)�ł͈قȂ郋�[�g�Ԃ̈ړ��͕s��
	 *  - ���l�[��/�ړ���ɓ����t�@�C������������L�����Z������
	 * @param[in] path �t�@�C���E�t�H���_�p�X
	 * @return 
	 */
	inline bool Rename(const std::string &from, const std::string &to)
	{
		fspath fp0(from);
		fspath fp1(to);

		if(!boost::filesystem::exists(fp0) || boost::filesystem::exists(fp1)){
			return false;
		}

		boost::filesystem::rename(fp0, fp1);

		return true;
	}

	/*!
	 * �t�@�C���R�s�[(�t�H���_�s��)
	 *  - �R�s�[��ɓ����t�@�C������������L�����Z������
	 * @param[in] path �t�@�C���p�X
	 * @return 
	 */
	inline bool Copy(const std::string &from, const std::string &to)
	{
		fspath fp0(from);
		fspath fp1(to);

		if(!boost::filesystem::exists(fp0) || boost::filesystem::exists(fp1)){
			return false;
		}

		boost::filesystem::copy_file(fp0, fp1);

		return true;
	}

	/*!
	 * ��e�L�X�g�쐬
	 *  - �t�H���_���Ȃ���΍쐬
	 * @param[in] path �t�@�C���p�X
	 */
	inline bool ZeroText(const std::string &path)
	{
		fspath file_path(path);

		// �t�H���_�p�X�̒��o
		fspath dir_path = file_path.branch_path();

		// �t�H���_���݊m�F
		if(!boost::filesystem::exists(dir_path)){
			// �t�H���_�쐬
			boost::filesystem::create_directory(dir_path);

			if(!boost::filesystem::exists(dir_path)){
				return false;
			}
		}

		FILE *fp = fopen(path.c_str(), "r");
		if(fp){
			fclose(fp);
			return false;
		}
		else{
			fp = fopen(path.c_str(), "w");
			if(fp){
				fclose(fp);
				return true;
			}
			else{
				return false;
			}
		}
	}

	/*!
	 * �t�@�C���p�X����t�@�C�����̂ݒ��o
	 * @param[in] path �t�@�C���p�X
	 */
	inline std::string FileName(const std::string &path)
	{
		fspath file_path(path);
		return file_path.filename().generic_string();
	}

	/*!
	 * �t�@�C���p�X����t�@�C�������̂��������̂𒊏o
	 * @param[in] path �t�@�C���p�X
	 */
	inline std::string DirName(const std::string &path)
	{
		fspath file_path(path);
		return file_path.branch_path().string();
	}

	/*!
	 * �t�@�C���p�X����e�t�H���_���𒊏o
	 * @param[in] path �t�@�C���p�X
	 */
	inline std::string ParentDirName(const std::string &path)
	{
		std::string::size_type pos1, pos0;
		pos1 = path.find_last_of("\\/");
		pos0 = path.find_last_of("\\/", pos1-1);

		if(pos0 != std::string::npos && pos1 != std::string::npos){
			return path.substr(pos0+1, pos1-pos0-1);
		}
		else{
			return "";
		}
	}

	/*!
	 * �t�H���_��؂�̌���
	 * @param[in] str �t�@�C���E�t�H���_�p�X
	 * @param[out] pos ���������ʒu
	 */
	inline bool FindPathBound(const string &str, std::string::size_type &pos)
	{
		std::string::size_type pos0, pos1;
		pos0 = str.find_last_of("\\");
		pos1 = str.find_last_of("/");

		if(pos0 == std::string::npos){
			if(pos1 == std::string::npos){
				return false;
			}
			else{
				pos = pos1;
			}
		}
		else{
			if(pos1 == std::string::npos){
				pos = pos0;
			}
			else{
				pos = (pos0 < pos1) ? pos0 : pos1;
			}
		}

		return true;
	}

	/*!
	 * �t�H���_�̑��݊m�F�ƍ쐬
	 * @param[in] path_str �t�@�C���E�t�H���_�p�X
	 */
	inline bool CheckAndMakeDir(const std::string &path_str)
	{
		using namespace boost::filesystem;

		if(!exists(path_str)){
			vector<string> tmp_paths;
			tmp_paths.push_back(path_str);

			// �p�X�𕪉�
			for(;;){
				std::string::size_type pos;
				if(!FindPathBound(tmp_paths.back(), pos)){
					break;
				}
				else{
					string str = tmp_paths.back().substr(0, pos);
					tmp_paths.push_back(str);
				}
			}

			// ���������p�X���ꂼ�ꑶ�݃`�F�b�N���C�Ȃ���΍쐬
			vector<std::string>::iterator itr = tmp_paths.end()-1;
			for( ; itr != tmp_paths.begin(); --itr){
				//cout << *itr << endl;
				if(!exists(*itr)){
					create_directory(*itr);
				}
			}
		}

		return true;
	}

	/*!
	 * �t�@�C���p�X����g���q���o(.(�h�b�g)�t)
	 * @param[in] str �t�@�C���p�X
	 * @return �g���q(.(�h�b�g)�t)
	 */
	inline string GetExtensionWithDot(const std::string &str)
	{
		std::string::size_type pos = str.find_last_of(".");
		return str.substr(pos, str.size());
	}

	/*!
	 * �t�@�C���p�X����g���q���o
	 * @param[in] str �t�@�C���p�X
	 * @return �g���q
	 */
	inline string GetExtension(const std::string &str)
	{
		std::string::size_type pos = str.find_last_of(".");
		return str.substr(pos+1, str.size());
	}


};	// namespace RXFile



#endif // #ifndef _RX_COMMON_FILE_H_