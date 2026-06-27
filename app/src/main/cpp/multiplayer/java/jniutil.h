#pragma once

#include "main.h"

#define EXCEPTION_CHECK(env) \
	if ((env)->ExceptionCheck()) \
	{ \
		(env)->ExceptionDescribe(); \
		(env)->ExceptionClear(); \
		return; \
	}

class CJavaWrapper
{
public:
    CJavaWrapper(JNIEnv *env, jobject activity);
    ~CJavaWrapper() {};

    const char* GetClipboardString();
    void SetPauseState(bool pause);
    void ShowLoadingScreen();
    void ShowNotification(int type, char *text, int duration, char *actionforBtn, char *textBtn);
    void HideLoadingScreen();
    void ShowTab();
	void ShowSpawn();
    void HideTab();
    void SetTab(int id, const char* name, int level, int ping, uint32_t color);
    void ClearTab();
    void CommitTab();
    void ShowKeyboard();
    void HideKeyboard();
	void exitGame();

    jbyteArray as_byte_array(unsigned char* buf, int len) {
        JNIEnv* p;
        javaVM->GetEnv((void**)&p, JNI_VERSION_1_6);
		jbyteArray array = p->NewByteArray (len);
		p->SetByteArrayRegion (array, 0, len, reinterpret_cast<jbyte*>(buf));
		return array;
	}

    void ShowDialog(int dialogStyle, int dialogID, char* title, char* text, char* button1, char* button2);

	void ShowEditObject();
	void HideEditObject();

    jobject activity;
    jmethodID s_setPauseState;
    jmethodID s_showLoadingScreen;
    jmethodID s_hideLoadingScreen;
    jmethodID s_ShowDialog;
    jmethodID s_showTab;
    jmethodID s_setTab;
    jmethodID s_clearTab;
    jmethodID s_commitTab;
    jmethodID s_hideTab;
    jmethodID s_showInputLayout;
    jmethodID s_hideInputLayout;
	jmethodID s_exitGame;
	jmethodID s_showEditObject;
	jmethodID s_hideEditObject;
	jmethodID s_showSpawn;
    jmethodID s_showNotification;

	static JNIEnv *GetEnv();
};

extern "C" {
JNIEXPORT void JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_sendCommand(JNIEnv* pEnv, jobject thiz, jbyteArray str);
}