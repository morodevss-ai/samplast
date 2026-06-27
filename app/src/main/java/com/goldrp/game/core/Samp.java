package com.goldrp.game.core;

import android.os.Bundle;
import android.util.Log;

import com.goldrp.game.ui.attach.AttachEdit;
import com.goldrp.game.ui.dialog.DialogManager;
import com.goldrp.game.ui.hud.Hud;
import com.goldrp.game.ui.hud.Speedometr;
import com.goldrp.game.ui.hud.Keyboard;
import com.goldrp.game.ui.hud.Notification;
import com.goldrp.game.ui.loading.LoadingScreen;
import com.goldrp.game.ui.tab.PlayerData;
import com.goldrp.game.ui.tab.Tab;
import com.goldrp.game.ui.reg.Reg;
import com.goldrp.game.ui.auth.SpawnSelector;
import com.nvidia.devtech.HeightProvider;

import android.os.Handler;
import android.widget.EditText;
import android.widget.FrameLayout;

import androidx.appcompat.app.AppCompatActivity;

import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import kotlin.jvm.internal.Intrinsics;

public class Samp extends GTASA implements Keyboard.InputListener, HeightProvider.HeightListener {
    private static final String TAG = "Samp";
    private static Samp instance;
    public static final Companion Companion = new Companion();
    private DialogManager mDialog;
    private FrameLayout mBackUILayout = null;
    private Keyboard mKeyboard;
    private LoadingScreen mLoadingScreen;
    private AttachEdit mAttachEdit;
    private HeightProvider mHeightProvider;
    private Tab mTabOverlay;
    private Speedometr mSpeedometr;
    private Notification mNotification;
    private Hud mHud;
    private Reg mReg = null;
    private SpawnSelector mSpawnSelector = null;
    private final Object mTabLock = new Object();
    private final HashMap<Integer, PlayerData> mTabPending = new HashMap<>();
    private boolean mIsTabInitialized = false;
    private boolean mIsClosingTab = false;
    private boolean mIsConnected = false;

    private Handler mHandler = new Handler();

    public static Samp getInstance() {
        return instance;
    }
    public static AppCompatActivity activity;
    private native void onChatInputRequested();
    private native void onKeyboardHidden();
    private native void onWeaponChanged();
    public native void sendSpawnClick(int id);

    public static final AppCompatActivity getActivity() {
        return Companion.getActivity();
    }

    private void showKeyboard()
    {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Log.d("AXL", "showKeyboard()");
                if(mKeyboard != null) {
                    mKeyboard.ShowInputLayoutForChatBottom();
                } else {
                    mKeyboard.ShowInputLayout();
                }
            }
        });
    }

    private void hideKeyboard()
    {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mKeyboard.HideInputLayout();
            }
        });
    }

    private void showEditObject() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (mAttachEdit != null) {
                    mAttachEdit.show();
                }
            }
        });
    }

    private void hideEditObject() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (mAttachEdit != null) {
                    mAttachEdit.hide();
                }
            }
        });
    }

    private void showTab()
    {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (mTabOverlay != null) {
                    mTabOverlay.show(true);
                }
            }
        });
    }

    private void hideTab()
    {
        if (mIsClosingTab) {
            return;
        }

        mIsClosingTab = true;

        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    if (mTabOverlay != null) {
                        mTabOverlay.close(false);
                    }
                } finally {
                    mIsClosingTab = false;
                }
            }
        });
    }

    private void setTab(int id, String name, int level, int ping, int color)
    {
        synchronized (mTabLock) {
            mTabPending.put(id, new PlayerData(id, name, level, ping));
        }
    }

    private void clearTab()
    {
        synchronized (mTabLock) {
            mTabPending.clear();
        }
    }

    private void commitTab()
    {
        final List<PlayerData> snapshot = new ArrayList<>();
        synchronized (mTabLock) {
            snapshot.addAll(mTabPending.values());
        }
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (mTabOverlay != null) {
                    mTabOverlay.setStats(snapshot);
                }
            }
        });
    }

    public void requestTabClose() {
        try {
            onTabClosed();
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, e.getMessage());
        }
    }

    private void showLoadingScreen() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (mLoadingScreen != null) {
                    mLoadingScreen.show();
                    mHandler.postDelayed(new Runnable() {
                        @Override
                        public void run() {
                            if (mLoadingScreen != null && mLoadingScreen.isVisible()) {
                                mLoadingScreen.hide();
                            }
                        }
                    }, 50000);
                }
            }
        });
    }

    private void hideLoadingScreen() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (mLoadingScreen != null && mLoadingScreen.isVisible()) {
                    mLoadingScreen.hide();
                }
            }
        });
    }

    public void onWeaponChangedFromUi() {
        try {
            onWeaponChanged();
        } catch (UnsatisfiedLinkError e5) {
            Log.e(TAG, e5.getMessage());
        }
    }

    public void onKeyboardHiddenFromUi() {
        try {
            onKeyboardHidden();
        } catch (UnsatisfiedLinkError e5) {
            Log.e(TAG, e5.getMessage());
        }
    }

    public void showDialogKeyboard(EditText target) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (mKeyboard != null) {
                    mKeyboard.setExternalTarget(target);
                    mKeyboard.ShowInputLayoutForDialog();
                }
            }
        });
    }

    public void showNotification(int type, String text, int duration, String actionforBtn, String textBtn) {
        runOnUiThread(() -> mNotification.ShowNotification(type, text, duration, actionforBtn, textBtn));
    }


    public void submitDialogFromKeyboard() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (mDialog != null && mDialog.isShow) {
                    mDialog.SendDialogResponse(1, mDialog.mCurrentListitem, mDialog.mCurrentInputtext);
                }
            }
        });
    }

    public void ShowSpawn() { runOnUiThread(() -> { mSpawnSelector.showselect(); });}
    public void HideSpawn() { runOnUiThread(() -> { mSpawnSelector.hide();});}

    public void setPauseState(boolean pause) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (pause) {
                    if (mDialog != null) {
                        mDialog.hideWithoutReset();
                    }
                    if (mAttachEdit != null) {
                        mAttachEdit.hideWithoutReset();
                    }
                } else {
                    if (mDialog != null && mDialog.isShow) {
                        mDialog.showWithOldContent();
                    }
                    if (mAttachEdit != null && mAttachEdit.isShow) {
                        mAttachEdit.showWithoutReset();
                    }
                }
            }
        });
    }

    public FrameLayout getBackUILayout() {
        return this.mBackUILayout;
    }

    public void setBackUILayout(FrameLayout layout) {
        this.mBackUILayout = layout;
        Log.i(TAG, "BackUILayout set: " + (layout != null));
    }

    public void exitGame(){
        finishAndRemoveTask();
        System.exit(0);
    }

    public void setConnected(boolean connected) {
        mIsConnected = connected;
        Log.i(TAG, "Connection state changed: " + connected);
    }

    public void showDialog(int dialogId, int dialogTypeId, byte[] bArr, byte[] bArr2, byte[] bArr3, byte[] bArr4) {
        final String caption = new String(bArr);
        final String content = new String(bArr2);
        final String leftBtnText = new String(bArr3);
        final String rightBtnText = new String(bArr4);
        runOnUiThread(() -> { this.mDialog.show(dialogId, dialogTypeId, caption, content, leftBtnText, rightBtnText); });
    }

    public void setServerName(String serverName) {
        runOnUiThread(() -> {
            if (mTabOverlay != null) {
                mTabOverlay.setServerName(serverName);
            }
        });
    }

    private native void onInputEnd(byte[] str);
    private native void onTabClosed();

    @Override
    public void OnInputEnd(String str) {
        byte[] toReturn = null;
        try {
            toReturn = str.getBytes("windows-1251");
        } catch (UnsupportedEncodingException e) {
        }

        try {
            onInputEnd(toReturn);
        } catch (UnsatisfiedLinkError e5) {
            Log.e(TAG, e5.getMessage());
        }
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        Log.i(TAG, "**** onCreate");
        super.onCreate(savedInstanceState);

        mBackUILayout = new FrameLayout(this);
        addContentView(mBackUILayout, new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                FrameLayout.LayoutParams.MATCH_PARENT
        ));

        mDialog = new DialogManager(this);
        mKeyboard = new Keyboard(this);
        mLoadingScreen = new LoadingScreen(this);
        mAttachEdit = new AttachEdit(this);
        mSpeedometr = new Speedometr(this);
        mSpeedometr.setContainer(mBackUILayout);
        mHud = new Hud(this);
        mNotification = new Notification(this);
        mReg = new Reg(this);
        mSpawnSelector = new SpawnSelector(this);
        mIsConnected = false;

        instance = this;
        mHud.attachToContainer(mBackUILayout);

        showLoadingScreen();

        try {
            initializeSAMP();
        } catch (UnsatisfiedLinkError e5) {
            Log.e(TAG, e5.getMessage());
        }
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus && !mIsTabInitialized) {
            mTabOverlay = new Tab(this);
            mIsTabInitialized = true;
            Log.i(TAG, "Tab initialized on window focus changed");
        }
    }

    private native void initializeSAMP();

    @Override
    public void onStart() {
        Log.i(TAG, "**** onStart");
        super.onStart();
    }

    @Override
    public void onRestart() {
        Log.i(TAG, "**** onRestart");
        super.onRestart();
    }


    @Override
    public void onResume() {
        Log.i(TAG, "**** onResume");
        super.onResume();
    }

    public native void onEventBackPressed();

    @Override
    public void onBackPressed() {
        super.onBackPressed();
        onEventBackPressed();
    }

    @Override
    public void onPause() {
        Log.i(TAG, "**** onPause");
        super.onPause();
    }

    @Override
    public void onStop() {
        Log.i(TAG, "**** onStop");
        super.onStop();
    }

    @Override
    public void onDestroy() {
        Log.i(TAG, "**** onDestroy");
        super.onDestroy();
        if (mLoadingScreen != null) {
            mLoadingScreen.destroy();
        }
    }

    @Override
    public void onHeightChanged(int orientation, int height) {
        //mDialog.onHeightChanged(height);
    }

    public static final class Companion {

        public static final Companion INSTANCE = new Companion();

        private Companion() {}

        public final AppCompatActivity getActivity() {
            AppCompatActivity appCompatActivity = activity;
            if (appCompatActivity != null) {
                return appCompatActivity;
            }
            Intrinsics.throwUninitializedPropertyAccessException("activity");
            return null;
        }

        public final void setActivity(AppCompatActivity appCompatActivity) {
            Intrinsics.checkNotNullParameter(appCompatActivity, "<set-?>");
            activity = appCompatActivity;
        }
    }
}