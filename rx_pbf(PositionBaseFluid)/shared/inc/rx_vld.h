/*!
  @file rx_vld.h
	
  @brief �{�����[���f�[�^�̓��o��
 
  @author Makoto Fujisawa
  @date   2013-10
*/

#ifndef _RX_VLD_H_
#define _RX_VLD_H_


//-----------------------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------------------
#include <iostream>
#include <fstream>
#include <sstream>
 
#include <cstdlib>

//-----------------------------------------------------------------------------
// Name Space
//-----------------------------------------------------------------------------
using namespace std;


//-----------------------------------------------------------------------------
// rxVLD�N���X�̐錾 - rxVLD�`���̓ǂݍ���
//-----------------------------------------------------------------------------
template<class T>
class rxVLD
{
	// �����o�֐�
	inline int ix(const int &i, const int &j, const int &k, const int &nx, const int &ny)
	{
		return (i+nx*(j+ny*k));
	}

public:
	//! �R���X�g���N�^
	rxVLD(){}
	//! �f�X�g���N�^
	~rxVLD(){}

	/*!
	 * VLD�t�@�C���ǂݍ���
	 * @param[in] file_name �t�@�C����(�t���p�X)
	 * @param[out] nx,ny,nz �Z����
	 * @return �{�����[���f�[�^
	 */
	T* Read(string file_name, int &nx, int &ny, int &nz);

	/*!
	 * VLD�t�@�C����������(������)
	 * @param[in] file_name �t�@�C����(�t���p�X)
	 * @param[in] nx,ny,nz �Z����
	 * @param[in] �{�����[���f�[�^
	 */
	bool Save(string file_name, int nx, int ny, int nz, T* data);
};

template<class T> 
T* rxVLD<T>::Read(string file_name, int &nx, int &ny, int &nz)
{
	// �t�@�C�����o�C�i�����[�h�ŊJ��
	ifstream fin;
	fin.open(file_name.c_str(), ios::in|ios::binary);
	if(!fin){
		cout << file_name << " could not open!" << endl;
		return 0;
	}

	// �Z�����̓ǂݍ���
	fin.read((char*)&nx, sizeof(int));
	fin.read((char*)&ny, sizeof(int));
	fin.read((char*)&nz, sizeof(int));

	if(nx == 0 || ny == 0 || nz == 0) return 0;

	T *data = new T[nx*ny*nz];

	// �{�����[���f�[�^�̓ǂݍ���
	for(int k = 0; k < nz; ++k){
		for(int j = 0; j < ny; ++j){
			for(int i = 0; i < nx; ++i){
				int idx = ix(i, j, k, nx, ny);
				fin.read((char*)&data[idx], sizeof(T));
			}
		}
	}

	fin.close();

	return data;
}

template<class T> 
bool rxVLD<T>::Save(string file_name, int nx, int ny, int nz, T* data)
{
	// �t�@�C�����o�C�i�����[�h�ŊJ��
	ofstream fout;
	fout.open(file_name.c_str(), ios::out|ios::binary);
	if(!fout){
		cout << file_name << " could not open!" << endl;
		return false;
	}

	// �Z�����̏�������
	fout.write((char*)&nx, sizeof(int));
	fout.write((char*)&ny, sizeof(int));
	fout.write((char*)&nz, sizeof(int));

	// �{�����[���f�[�^�̏�������
	for(int k = 0; k < nz; ++k){
		for(int j = 0; j < ny; ++j){
			for(int i = 0; i < nx; ++i){
				int idx = ix(i, j, k, nx, ny);
				fout.write((char*)&data[idx], sizeof(T));
			}
		}
	}

	fout.close();

	return true;
}



#endif // _RX_VLD_H_
