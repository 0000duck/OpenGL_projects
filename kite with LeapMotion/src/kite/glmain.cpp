/*! 
  @file glmain.cpp
	
  @simulation interactive kite using Leapmotion

  @date 2015
*/

#pragma comment (lib, "glew32.lib")



//-----------------------------------------------------------------------------
// �C���N���[�h�t�@�C��
//-----------------------------------------------------------------------------
// STL
#include <vector>
#include <string>
//Leapmotion
#include "Leap.h"
// OpenGL
#include <GL/glew.h>
#include <GL/glut.h>

#include "rx_utility.h"
#include "rx_trackball.h"

#include "rx_kite.h"

using namespace std;
using namespace Leap;

//-----------------------------------------------------------------------------
// ��`/�萔
//-----------------------------------------------------------------------------
const string RX_PROGRAM_NAME = "gl_simple";


//-----------------------------------------------------------------------------
// �O���[�o���ϐ�
//-----------------------------------------------------------------------------
// �E�B���h�E���
int g_iWinW = 720;		//!< �`��E�B���h�E�̕�
int g_iWinH = 720;		//!< �`��E�B���h�E�̍���
int g_iWinX = 100;		//!< �`��E�B���h�E�ʒux
int g_iWinY = 100;		//!< �`��E�B���h�E�ʒuy

bool g_bIdle = false;	//!< �A�C�h�����

//! �w�i�F
double g_fBGColor[4] = {0.8, 0.9, 1.0, 1.0};

//! �g���b�N�{�[���ɂ�鎋�_�ړ�
rxTrackball g_tbView;

extern int nsteps;
extern Vec3 spring_ce;
extern kite3d::kite_3d kite;


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
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	// �I�u�W�F�N�g�`��
	glColor3d(1.0, 1.0, 1.0);
	kite3d::draw_kite();
	kite3d::draw_line();
	kite3d::draw_tail();

	glPushMatrix();
	double mag = kite.l_init+1.0;
	glTranslated(-mag, -mag, -mag);
	if(fluid::V_field) fluid::draw_velocity(2*mag, 0.1);	// ���x�̎��o��
		std:: cout<<"vvvvvvvvv field"<<mag<<endl;
	fluid::draw_box(2*mag);
	glPopMatrix();

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

	RenderScene();

	glPopMatrix();

	glutSwapBuffers();
}

/*! 
 * �A�C�h���C�x���g�����֐�
 */
void Idle(void)
{
	int n = GRID;
	double visc = VISC;
	double dt = STEP;

	// �ŏ����畗�����Ԃɂ������ꍇ
	//if(nsteps == 0){
	//	for(int i = 0; i < 30; ++i){
	//		fluid_get_from_UI(n, dens_prev, u_prev, v_prev, w_prev);
	//		fluid_vel_step(n, u, v, w, u_prev, v_prev, w_prev, visc, Dt);
	//	}
	//}

	fluid::get_from_UI(n, g_dens_prev, g_u_prev, g_v_prev, g_w_prev);
	fluid::vel_step(n, g_u, g_v, g_w, g_u_prev, g_v_prev, g_w_prev, visc, dt);
	kite3d::step_simulation(dt);

	spring_ce = Vec3(0.0, 0.0, 0.0);
	nsteps++;

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
	g_tbView.SetScaling(-20.0);
	//g_tbView.SetTranslation(0.0, -2.0);


	kite3d::read_file("alpha_CD_068.dat");
	kite3d::read_file("alpha_CD_148.dat");
	kite3d::read_file("alpha_CL_068.dat");
	kite3d::read_file("alpha_CL_148.dat");
	kite3d::read_file("alpha_x.dat");

	//�����f���̏�����
	kite3d::initialize_options();		//�I�v�V�����̏�����
	kite3d::initialize_sim();			//���p�����[�^�̏�����
	kite3d::create_model_rec();			//�����`���f���̍쐬
	//kite3d::create_model_yak();		//��������f���̍쐬
	//kite3d::create_model_dia();		//�H�`���f���̍쐬
	kite3d::initialize_deflection();		//����݂̏�����

	// ���̃V�~�����[�^�̏�����
	//initialize_fluid_sim
	fluid::allocate_data();
	fluid::clear_data();


	spring_ce=Vec3();
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

	case 'i': //������
		kite3d::initialize_sim();
		break;

	case 'x': //x�����̕�
		fluid::Z_wind=0;
		if(fluid::X_wind==0){		
			fluid::X_wind=1;
		}
		break;

	case 'z': //z�����̕�
		fluid::X_wind=0;
		if(fluid::Z_wind == 0){
			fluid::Z_wind = 1;
		}
		break;

	case 'u': //x������l��
		fluid::X_wind = 0;
		fluid::Z_wind = 0;
		break;

	case 'v': //���x�̎��o��ON/OFF
		fluid::V_field ^= 1;
		break;

	case 'd':
		fluid::D_tex ^= 1;
		break;

	case 'f':
		fluid::frc_view ^= 1;
		break;

	case 'h':
		cout << "'s' key : simulation start/stop" << endl;
		cout << "'F' key : fullscreen" << endl;
		cout << "'i' key : initialize the scene" << endl;
		cout << "'x','y','z' key : change the wind direction" << endl;
		cout << "'v' key : visualize the wind field" << endl;
		break;

	default:
		break;
	}

	glutPostRedisplay();
}

/*!
 * ����L�[�{�[�h�C�x���g�����֐�
 * @param[in] key �L�[�̎��
 * @param[in] x,y �L�[�������ꂽ�Ƃ��̃}�E�X���W(�X�N���[�����W�n)
 */

void SpecialKey(int key, int x, int y)
{
	double d = 100.0;
	switch(key){
	case GLUT_KEY_LEFT:
		spring_ce = Vec3(0.0, 0.0, -d);
		break;

	case GLUT_KEY_RIGHT:
		spring_ce = Vec3(0.0, 0.0,  d);
		break;

	case GLUT_KEY_UP:
		spring_ce = Vec3(d, 0.0, 0.0);
		break;

	case GLUT_KEY_DOWN:
		spring_ce = Vec3(-d, 0.0, 0.0);
		break;

	default:
		break;
	}

	glutPostRedisplay();
}
//Leapmotion setting 
class LeapMotionListener : public Listener
{
public:
    virtual void onInit(const Controller&);
    virtual void onConnect(const Controller&);
    virtual void onDisconnect(const Controller&);
    virtual void onExit(const Controller&);
    virtual void onFrame(const Controller&);
private:
};

void LeapMotionListener::onInit(const Controller& controller) {
  std::cout << "Initialized" << std::endl;
}

void LeapMotionListener::onExit(const Controller& controller) {
  std::cout << "Exited" << std::endl;
}

void LeapMotionListener::onConnect(const Controller& controller) {
  std::cout << "Connected" << std::endl;
  controller.enableGesture(Gesture::TYPE_CIRCLE);
  controller.enableGesture(Gesture::TYPE_KEY_TAP);
  controller.enableGesture(Gesture::TYPE_SCREEN_TAP);
  controller.enableGesture(Gesture::TYPE_SWIPE);
}

void LeapMotionListener::onDisconnect(const Controller& controller) {
  // Note: not dispatched when running in a debugger.
  std::cout << "Disconnected" << std::endl;
  controller.config().setFloat("Gesture.KeyTap.MinDownVelocity", 40.0);
	controller.config().setFloat("Gesture.KeyTap.HistorySeconds", .2);
	controller.config().setFloat("Gesture.KeyTap.MinDistance", 8.0);
	controller.config().save();

	controller.config().setFloat("Gesture.Swipe.MinLength", 200.0);
	controller.config().setFloat("Gesture.Swipe.MinVelocity", 750);
	controller.config().save();

	controller.config().setFloat("Gesture.ScreenTap.MinForwardVelocity", 30.0);
	controller.config().setFloat("Gesture.ScreenTap.HistorySeconds", .5);
	controller.config().setFloat("Gesture.ScreenTap.MinDistance", 1.0);
	controller.config().save();
}

void LeapMotionListener::onFrame(const Controller& controller) {
//	  std::cout << "Frame available" << std::endl;
	// Get the most recent frame and report some basic information
	const Frame frame = controller.frame();

	 HandList hands = frame.hands();
	 for (HandList::const_iterator hl = hands.begin(); hl != hands.end(); ++hl) {
	// Get the first hand
	const Hand hand = *hl;
	std::string handType = hand.isLeft() ? "Left hand" : "Right hand";
	/*std::cout << std::string(2, ' ') << handType << ", id: " << hand.id()
				  << ", palm position: " << hand.palmPosition() << std::endl;*/
	
	//movepos = hand.palmVelocity();
	std::cout << "       speed                   "<<hand.palmVelocity()<<endl;
	spring_ce = Vec3(hand.palmVelocity().x,hand.palmVelocity().y,hand.palmVelocity().z);
	// Get the hand's normal vector and direction
	const Vector normal = hand.palmNormal();
	const Vector direction = hand.direction();
  }

	 // Get gestures
  const GestureList gestures = frame.gestures();
  for (int g = 0; g < gestures.count(); ++g) {
    Gesture gesture = gestures[g];

    switch (gesture.type()) {
      case Gesture::TYPE_CIRCLE:
      {
        CircleGesture circle = gesture;
        std::string clockwiseness;

        if (circle.pointable().direction().angleTo(circle.normal()) <= PI/2) {
          clockwiseness = "clockwise";
        } else {
          clockwiseness = "counterclockwise";

        }

        // Calculate angle swept since last frame
        float sweptAngle = 0;
        if (circle.state() != Gesture::STATE_START) {
          CircleGesture previousUpdate = CircleGesture(controller.frame(1).gesture(circle.id()));
          sweptAngle = (circle.progress() - previousUpdate.progress()) * 2 * PI;
        }

        break;
      }
      case Gesture::TYPE_SWIPE:
      {
        SwipeGesture swipe = gesture;

        break;
      }
      case Gesture::TYPE_KEY_TAP:
      {
        KeyTapGesture tap = gesture;
		fluid::V_field ^= 1;
        break;
      }
      case Gesture::TYPE_SCREEN_TAP:
      {
        ScreenTapGesture screentap = gesture;
		//SwitchIdle(-1);
        break;
      }
      default:
        std::cout << std::string(2, ' ')  << "Unknown gesture type." << std::endl;
        break;
    }
  }
}

/*! 
 * GL�̏I���֐�
 */
void CleanGL(void)
{
	fluid::free_data();
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
	glutAddMenuEntry("Toggle fullscreen [f]",		'f');
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

	// Create a sample listener and controller
	LeapMotionListener listener;
	Controller controller;
	// Have the sample listener receive events from the controller
  controller.addListener(listener);

  if (argc > 1 && strcmp(argv[1], "--bg") == 0)
    controller.setPolicy(Leap::Controller::POLICY_BACKGROUND_FRAMES);

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

	Keyboard('h', 0, 0);

	glutMainLoop();

	CleanGL();

	return 0;
}


