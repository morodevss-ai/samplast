package com.goldrp.game.ui.hud;

import android.app.Activity;
import android.os.CountDownTimer;
import android.view.View;
import android.view.animation.AnimationUtils;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.constraintlayout.widget.ConstraintLayout;

import com.nvidia.devtech.NvEventQueueActivity;
import com.goldrp.game.R;
import com.goldrp.game.launcher.util.Util;

import java.io.UnsupportedEncodingException;

public class Notification {
    public Activity aactivity;

    public ConstraintLayout notifyBg;
    public ImageView notifyIcon;
    public TextView notifyText;
    public TextView notifyButt;

    public static int type, duration;
    public static String text, actionforBtn, textBtn;

    public CountDownTimer countDownTimer;

    public Notification(Activity activity) {
        aactivity = activity;

        if (activity != null) {
            notifyBg = activity.findViewById(R.id.notify_bg);
            notifyIcon = activity.findViewById(R.id.notify_icon);
            notifyText = activity.findViewById(R.id.notify_text);
            notifyButt = activity.findViewById(R.id.notify_butt);
        }

        if (notifyBg != null) {
            notifyBg.setOnClickListener(v -> {
                HideNotification();
            });

            Util.HideLayout(notifyBg, false);
        } else {
            if (activity != null) {
                android.util.Log.e("Notification", "notifyBg not found in layout");
            }
        }
    }

    public void ShowNotification(int type, String text, int duration, String actionforBtn, String textBtn) {
        if (notifyBg == null || notifyIcon == null || notifyText == null || notifyButt == null) {
            android.util.Log.e("Notification", "Views not initialized, cannot show notification");
            return;
        }

        Util.HideLayout(notifyBg, false);
        clearData();

        this.type = type;
        this.text = text;
        this.duration = duration;
        this.actionforBtn = actionforBtn;
        this.textBtn = textBtn;

        this.notifyText.setText(Util.transfromColors(this.text));

        switch (this.type) {
            case 0:
                notifyButt.setVisibility(View.GONE);
                notifyIcon.setImageResource(R.drawable.notify_icon_error);
                notifyBg.setBackgroundResource(R.drawable.notify_bg_error);
                break;
            case 1:
                notifyButt.setVisibility(View.GONE);
                notifyIcon.setImageResource(R.drawable.notify_icon_success);
                notifyBg.setBackgroundResource(R.drawable.notify_bg_success);
                break;
            case 2:
                notifyButt.setVisibility(View.GONE);
                notifyIcon.setImageResource(R.drawable.notify_icon_error);
                notifyBg.setBackgroundResource(R.drawable.notify_bg_error);
                break;
            case 3:
                notifyButt.setVisibility(View.GONE);
                notifyIcon.setImageResource(R.drawable.notify_icon_success);
                notifyBg.setBackgroundResource(R.drawable.notify_bg_success);
                break;
            case 4:
                notifyButt.setVisibility(View.VISIBLE);
                notifyIcon.setImageResource(R.drawable.notify_icon_success);
                notifyBg.setBackgroundResource(R.drawable.notify_bg_success);
                break;
            case 5:
                notifyButt.setVisibility(View.VISIBLE);
                notifyIcon.setImageResource(R.drawable.notify_icon_error);
                notifyBg.setBackgroundResource(R.drawable.notify_bg_error);
                break;
        }

        if (this.type == 5 || this.type == 4) {
            this.notifyButt.setText(textBtn);
            this.notifyButt.setOnClickListener(view -> {
                view.startAnimation(AnimationUtils.loadAnimation(aactivity, R.anim.button_click));
                try {
                    NvEventQueueActivity.getInstance().sendClick((Notification.actionforBtn).getBytes("windows-1251"));
                } catch (UnsupportedEncodingException e) {
                    e.printStackTrace();
                }
                HideNotification();
            });
        }

        startCountdown();

        Util.ShowLayout(notifyBg, true);
    }

    private void clearData() {
        this.text = "";
        this.type = -1;
        this.duration = -1;
        this.actionforBtn = "";
        this.textBtn = "";
    }

    public void startCountdown() {
        if (countDownTimer != null) {
            countDownTimer.cancel();
            countDownTimer = null;
        }

        countDownTimer = new CountDownTimer(duration * 1000L, 100) {
            @Override
            public void onTick(long millisUntilFinished) {
            }

            @Override
            public void onFinish() {
                HideNotification();
            }
        }.start();
    }

    public void HideNotification() {
        if (countDownTimer != null) {
            countDownTimer.cancel();
            countDownTimer = null;
        }

        if (notifyBg != null) {
            notifyBg.startAnimation(AnimationUtils.loadAnimation(aactivity, R.anim.popup_hide_notification));
            notifyBg.setVisibility(View.GONE);
        }
    }
}