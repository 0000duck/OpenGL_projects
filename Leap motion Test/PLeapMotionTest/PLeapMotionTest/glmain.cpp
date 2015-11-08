/*! 
  @file glmain.cpp
	
  @brief GLUT��{�\��
 
  @author Makoto Fujisawa
  @date 2010-02
*/

#pragma comment (lib, "glew32.lib")


//-----------------------------------------------------------------------------
// �C���N���[�h�t�@�C��
//-----------------------------------------------------------------------------
#include "utils.h"
#include "rx_trackball.h"
#include "Leap.h"

using namespace Leap;
//-----------------------------------------------------------------------------
// ��`/�萔
//-----------------------------------------------------------------------------
const string RX_PROGRAM_NAME = "gl_simple";

// �`��t���O
enum
{
	RXD_VERTEX		= 0x0001,	//!< ���_�`��
	RXD_EDGE		= 0x0002,	//!< �G�b�W�`��
	RXD_FACE		= 0x0004,	//!< �ʕ`��
	RXD_NORMAL		= 0x0008,	//!< �@���`��
};
const string RX_DRAW_STR[] = {
	"Vertex",	"v", 
	"Edge",		"e", 
	"Face",		"f", 
	"Normal",	"n", 
	"-1"
};


//-----------------------------------------------------------------------------
// �O���[�o���ϐ�
//-----------------------------------------------------------------------------
// �E�B���h�E���
int g_iWinW = 720;		//!< �`��E�B���h�E�̕�
int g_iWinH = 720;		//!< �`��E�B���h�E�̍���
int g_iWinX = 100;		//!< �`��E�B���h�E�ʒux
int g_iWinY = 100;		//!< �`��E�B���h�E�ʒuy

bool g_bIdle = false;	//!< �A�C�h�����
int g_iDraw = RXD_FACE;	//!< �`��t���O

double g_fAng = 0.0;	//!< �`���]�p

//! �w�i�F
double g_fBGColor[4] = {0.0, 0.0, 0.0, 1.0};

//! �g���b�N�{�[���ɂ�鎋�_�ړ�
rxTrackball g_tbView;


//-----------------------------------------------------------------------------
// �֐��v���g�^�C�v�錾
//-----------------------------------------------------------------------------
void OnExitApp(int id = -1);
void Idle(void);
void CleanGL(void);



//-----------------------------------------------------------------------------
// �A�v���P�[�V��������
//-----------------------------------------------------------------------------

/*!
 * �A�C�h���֐���ON/OFF
 * @param[in] on true��ON, false��OFF
 */
void SwitchIdle(int on)
{
	g_bIdle = (on == -1) ? !g_bIdle : (on ? true : false);
	glutIdleFunc((g_bIdle ? Idle : 0));
	cout << "idle " << (g_bIdle ? "on" : "off") << endl;
}

/*!
 * �t���X�N���[��/�E�B���h�E�\���̐؂�ւ�
 */
void SwitchFullScreen(void)
{
	static int fullscreen = 0;		// �t���X�N���[�����
	static int pos0[2] = { 0, 0 };
	static int win0[2] = { 500, 500 };
	if(fullscreen){
		glutPositionWindow(pos0[0], pos0[1]);
		glutReshapeWindow(win0[0], win0[1]);
	}
	else{
		pos0[0] = glutGet(GLUT_WINDOW_X);
		pos0[1] = glutGet(GLUT_WINDOW_Y);
		win0[0] = glutGet(GLUT_WINDOW_WIDTH);
		win0[1] = glutGet(GLUT_WINDOW_HEIGHT);

		glutFullScreen();
	}
	fullscreen ^= 1;
}

/*!
 * �A�v���P�[�V�����I�� �� Quit�{�^���̃C�x���g�n���h��
 * @param[in] id �{�^���̃C�x���gID
 */
void OnExitApp(int id)
{
	CleanGL();

	exit(1);
}


//-----------------------------------------------------------------------------
// �`��֐��Ȃ�
//-----------------------------------------------------------------------------
/*!
 * �������e�ϊ�
 */
void Projection(void)
{
	gluPerspective(45.0f, (float)g_iWinW/(float)g_iWinH, 0.2f, 1000.0f);
	//glOrtho(-1, 1, -1, 1, -1, 1);
	//gluOrtho2D(0, g_iWinW, 0, g_iWinH);
}

void RenderScene(void)
{
	glPushMatrix();

	glEnable(GL_LIGHTING);

	switch(g_iDraw){
	case RXD_FACE: glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); break;
	case RXD_EDGE: glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); break;
	case RXD_VERTEX: glPolygonMode(GL_FRONT_AND_BACK, GL_POINT); break;
	}

	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	// �I�u�W�F�N�g�`��
	glColor3d(1.0, 1.0, 1.0);
	glutSolidTeapot(1.0);

	glPopMatrix();
}


//------------------------------------------------------------------
// OpenGL�L�����o�X�p�̃C�x���g�n���h��
//------------------------------------------------------------------
/*!
 * �ĕ`��C�x���g�����֐�
 */
void Display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glPushMatrix();

	g_tbView.Apply();	// �}�E�X�ɂ���]�E���s�ړ��̓K�p

	glPushMatrix();
	glRotated(g_fAng, 0, 1, 0);
	RenderScene();
	glPopMatrix();

	glPopMatrix();

	// ������`��
	vector<string> strs;
	strs.push_back("\"s\" key : idle on/off");
	strs.push_back("SHIFT+\"f\" key : fullscreen on/off");
	strs.push_back("\"v\",\"e\",\"f\" key : switch vertex,edge,face drawing");
	glColor3d(1.0, 1.0, 1.0);
	DrawStrings(strs, g_iWinW, g_iWinH);

	glutSwapBuffers();
}


/*! 
 * ���T�C�Y�C�x���g�����֐�
 * @param[in] w �L�����o�X��(�s�N�Z����)
 * @param[in] h �L�����o�X����(�s�N�Z����)
 */
void Resize(int w, int h)
{
	g_iWinW = w;
	g_iWinH = h;

	glViewport(0, 0, w, h);
	g_tbView.SetRegion(w, h);

	// �����ϊ��s��̐ݒ�
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	Projection();

	// ���f���r���[�ϊ��s��̐ݒ�
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

/*!
 * �}�E�X�C�x���g�����֐�
 * @param[in] button �}�E�X�{�^��(GLUT_LEFT_BUTTON,GLUT_MIDDLE_BUTTON,GLUT_RIGHT_BUTTON)
 * @param[in] state �}�E�X�{�^���̏��(GLUT_UP, GLUT_DOWN)
 * @param[in] x,y �}�E�X���W(�X�N���[�����W�n)
 */
void Mouse(int button, int state, int x, int y)
{
	if(x < 0 || y < 0) return;

	int mod = glutGetModifiers();	// SHIFT,CTRL,ALT�̏�Ԏ擾

	if(button == GLUT_LEFT_BUTTON){
		if(state == GLUT_DOWN){
			g_tbView.Start(x, y, mod+1);
		}
		else if(state == GLUT_UP){
			g_tbView.Stop(x, y);
		}
	}
	else if(button == GLUT_MIDDLE_BUTTON){
	}
	else if(button == GLUT_RIGHT_BUTTON){
	}

	glutPostRedisplay();
}

/*!
 * ���[�V�����C�x���g�����֐�(�}�E�X�{�^�����������܂܃h���b�O)
 * @param[in] x,y �}�E�X���W(�X�N���[�����W�n)
 */
void Motion(int x, int y)
{
	if(x < 0 || y < 0) return;
	int mod = glutGetModifiers();	// SHIFT,CTRL,ALT�̏�Ԏ擾
	g_tbView.Motion(x, y);
	glutPostRedisplay();
}

/*!
 * ���[�V�����C�x���g�����֐�(�}�E�X�{�^���������Ȃ��ړ�)
 * @param[in] x,y �}�E�X���W(�X�N���[�����W�n)
 */
void PassiveMotion(int x, int y)
{
	if(x < 0 || y < 0) return;
}


/*! 
 * �A�C�h���C�x���g�����֐�
 */
void Idle(void)
{
	g_fAng += 0.5;
	glutPostRedisplay();
}

/*!
 * �L�[�{�[�h�C�x���g�����֐�
 * @param[in] key �L�[�̎��
 * @param[in] x,y �L�[�������ꂽ�Ƃ��̃}�E�X���W(�X�N���[�����W�n)
 */
void Keyboard(unsigned char key, int x, int y)
{
	int mod = glutGetModifiers();	// SHIFT,CTRL,ALT�̏�Ԏ擾

	switch(key){
	case '\033':  // '\033' �� ESC �� ASCII �R�[�h
		OnExitApp();
		break;

	case ' ':	// �A�j���[�V����1�X�e�b�v�������s
		Idle();
		break;

	case 's':	// �A�j���[�V����ON/OFF
		SwitchIdle(-1);
		break;

	case 'F':	// �t���X�N���[��
		SwitchFullScreen();
		break;

	default:	// �`��t���O�̏���
		{
			int idx = 0;
			while(RX_DRAW_STR[2*idx] != "-1" && (int)RX_DRAW_STR[2*idx+1][0] != key) idx++;
			if(RX_DRAW_STR[2*idx] != "-1"){
				//g_iDraw ^= (0x01 << idx);	// �G�b�W��ʂ𓯎��ɕ`�悷��ۂ͂�������g�p
				g_iDraw = (0x01 << idx);
			}
		}
		break;
	}

	glutPostRedisplay();
}

class LeapEventListener : public Listener {
  public:
    virtual void onConnect(const Controller&);
    virtual void onDisconnect(const Controller&);
    virtual void onFrame(const Controller&);
};


void LeapEventListener::onConnect(const Controller& controller) {
    std::cout << "Connected" << std::endl;
    // Enable gestures, set Config values:
    controller.enableGesture(Gesture::TYPE_SWIPE);
    controller.config().setFloat("Gesture.Swipe.MinLength", 200.0);
    controller.config().save();
}

//Not dispatched when running in a debugger
void LeapEventListener::onDisconnect(const Controller& controller) {
    std::cout << "Disconnected" << std::endl;
}

void LeapEventListener::onFrame(const Controller& controller) {
    std::cout << "New frame available" << std::endl;
    Frame frame = controller.frame();
    // Process this frame's data...
}
/*!
 * ����L�[�{�[�h�C�x���g�����֐�
 * @param[in] key �L�[�̎��
 * @param[in] x,y �L�[�������ꂽ�Ƃ��̃}�E�X���W(�X�N���[�����W�n)
 */
void SpecialKey(int key, int x, int y)
{
	switch(key){
	case GLUT_KEY_LEFT:
		break;

	case GLUT_KEY_RIGHT:
		break;

	case GLUT_KEY_UP:
		break;

	case GLUT_KEY_DOWN:
		break;

	default:
		break;
	}

	glutPostRedisplay();
}



/*! 
 * GL�̏������֐�
 */
void InitGL(void)
{
	cout << "OpenGL Ver. " << glGetString(GL_VERSION) << endl;

	GLenum err = glewInit();
	if(err == GLEW_OK){
		cout << "GLEW OK : Glew Ver. " << glewGetString(GLEW_VERSION) << endl;
	}
	else{
		cout << "GLEW Error : " << glewGetErrorString(err) << endl;
	}

	// �}���`�T���v�����O�̑Ή��󋵊m�F
	GLint buf, sbuf;
	glGetIntegerv(GL_SAMPLE_BUFFERS, &buf);
	cout << "number of sample buffers is " << buf << endl;
	glGetIntegerv(GL_SAMPLES, &sbuf);
	cout << "number of samples is " << sbuf << endl;

	glClearColor((GLfloat)g_fBGColor[0], (GLfloat)g_fBGColor[1], (GLfloat)g_fBGColor[2], 1.0f);
	glClearDepth(1.0f);

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glEnable(GL_MULTISAMPLE);

	// �|���S���@���ݒ�
	glEnable(GL_AUTO_NORMAL);
	glEnable(GL_NORMALIZE);

	// �����ݒ�
	static const GLfloat lpos[] = { 0.0f, 0.0f, 1.0f, 1.0f };	// �����ʒu
	static const GLfloat difw[] = { 0.7f, 0.7f, 0.7f, 1.0f };	// �g�U�F : ��
	static const GLfloat spec[] = { 0.2f, 0.2f, 0.2f, 1.0f };	// ���ʔ��ːF
	static const GLfloat ambi[] = { 0.1f, 0.1f, 0.1f, 1.0f };	// ����
	glLightfv(GL_LIGHT0, GL_POSITION, lpos);
	glLightfv(GL_LIGHT0, GL_DIFFUSE,  difw);
	glLightfv(GL_LIGHT0, GL_SPECULAR, spec);
	glLightfv(GL_LIGHT0, GL_AMBIENT,  ambi);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHTING);

	glShadeModel(GL_SMOOTH);

	// �g���b�N�{�[�������p��
	g_tbView.SetScaling(-5.0);
	//g_tbView.SetTranslation(0.0, -2.0);
}

/*! 
 * GL�̏I���֐�
 */
void CleanGL(void)
{
}

/*!
 * ���C�����j���[
 * @param[in] id ���j���[ID
 */
void OnMainMenu(int id)
{
	Keyboard((unsigned char)id, 0, 0);
}

/*!
 * GLUT�̉E�N���b�N���j���[�쐬
 */
void InitMenu(void)
{
	// ���C�����j���[
	glutCreateMenu(OnMainMenu);

	// �`�惁�j���[
	glutAddMenuEntry(" -------- draw --------- ", -1);
	int count = 0;
	while(RX_DRAW_STR[2*count] != "-1"){
		string label = RX_DRAW_STR[2*count];
		glutAddMenuEntry(label.c_str(),	RX_DRAW_STR[2*count+1][0]);
		count++;
	}
	glutAddMenuEntry(" ------------------------ ",	-1);
	glutAddMenuEntry("Toggle fullscreen [F]",		'F');
	glutAddMenuEntry("Toggle animation [s]",		's');
	glutAddMenuEntry("Step animation [ ]",			' ');
	glutAddMenuEntry(" ------------------------ ",	-1);
	glutAddMenuEntry("Quit [ESC]",					'\033');
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}



/*!
 * ���C�����[�`��
 * @param[in] argc �R�}���h���C�������̐�
 * @param[in] argv �R�}���h���C������
 */
int main(int argc, char *argv[])
{
Controller controller;
LeapEventListener listener;
controller.addListener(listener);




	glutInitWindowPosition(g_iWinX, g_iWinY);
	glutInitWindowSize(g_iWinW, g_iWinH);
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_ACCUM);
	glutCreateWindow(argv[0]);

	glutDisplayFunc(Display);
	glutReshapeFunc(Resize);
	glutMouseFunc(Mouse);
	glutMotionFunc(Motion);
	glutPassiveMotionFunc(PassiveMotion);
	glutKeyboardFunc(Keyboard);
	glutSpecialFunc(SpecialKey);

	SwitchIdle(0);


	InitGL();
	InitMenu();

	glutMainLoop();


	CleanGL();
	    // Remove the sample listener when done
    controller.removeListener(listener);
	return 0;
}


