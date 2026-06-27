package com.goldrp.game.ui.hud;

import android.app.Activity;
import android.os.Handler;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.TextView;

import com.goldrp.game.R;
import com.goldrp.game.core.Samp;
import com.goldrp.game.launcher.util.SeekArc;

public class Speedometr {
    private final int BUTTON_ENGINE = 0;
    private final int BUTTON_LIGHT = 1;
    private final int BUTTON_DOOR = 5;

    private View rootView;
    private boolean menuVisible = false;
    private FrameLayout customContainer; 

    private SeekArc speedLine;
    private SeekArc speedLine1;
    private ImageView engineStart;
    private TextView speedText;
    private TextView textView4;
    private SeekArc fuelLine;
    private SeekArc fuelLine2;
    private ImageView fuelType;
    private TextView fuelText;
    private ImageView speedEngine;
    private ImageView speedLights;
    private ImageView speedDoors;

    private Handler notificationHandler = new Handler();
    private Runnable hideNotificationRunnable;

    native void nativeSendClick(int id);
    native void nativeInit();
    private Activity activity;

    public Speedometr(Activity activity) {
        this.activity = activity;
        nativeInit();

        rootView = LayoutInflater.from(activity).inflate(R.layout.speedometr, null);

        initViews();
        setupClickListeners();
    }

    private void initViews() {
        speedLine = rootView.findViewById(R.id.speed_line);
        speedLine1 = rootView.findViewById(R.id.speed_line1);
        engineStart = rootView.findViewById(R.id.engine_start);
        speedText = rootView.findViewById(R.id.speed_text);
        textView4 = rootView.findViewById(R.id.textView4);
        fuelLine = rootView.findViewById(R.id.fuel_line);
        fuelLine2 = rootView.findViewById(R.id.fuel_line2);
        fuelType = rootView.findViewById(R.id.fuel_type);
        fuelText = rootView.findViewById(R.id.fuel_text);
        speedEngine = rootView.findViewById(R.id.speed_engine);
        speedLights = rootView.findViewById(R.id.speed_lights);
        speedDoors = rootView.findViewById(R.id.speed_doors);
    }

    private void setupClickListeners() {
        engineStart.setOnClickListener(v -> {
            nativeSendClick(BUTTON_ENGINE);
        });
    }

    public void setContainer(FrameLayout container) {
        this.customContainer = container;
    }

    public void showMenu(boolean show) {
        menuVisible = show;
    }

    public void viewVisible(ViewGroup viewGroup, int view) {
        if (viewGroup != null) {
            viewGroup.setAlpha(view == View.VISIBLE ? 1.0f : 0.0f);
            viewGroup.setVisibility(view);
        }
    }

    public void show() {
        activity.runOnUiThread(() -> {
            if (rootView.getParent() != null) {
                ((ViewGroup) rootView.getParent()).removeView(rootView);
            }

            FrameLayout container = null;

            if (customContainer != null) {
                container = customContainer;
            } else {
                Samp samp = Samp.getInstance();
                if (samp != null) {
                    container = samp.getBackUILayout();
                }
            }

            if (container != null) {
                container.addView(rootView, new FrameLayout.LayoutParams(
                        FrameLayout.LayoutParams.MATCH_PARENT,
                        FrameLayout.LayoutParams.MATCH_PARENT
                ));
                rootView.setVisibility(View.VISIBLE);
            } else {
                if (activity != null && activity.getWindow() != null) {
                    FrameLayout decorView = (FrameLayout) activity.getWindow().getDecorView();
                    decorView.addView(rootView, new FrameLayout.LayoutParams(
                            FrameLayout.LayoutParams.MATCH_PARENT,
                            FrameLayout.LayoutParams.MATCH_PARENT
                    ));
                    rootView.setVisibility(View.VISIBLE);
                }
            }
        });
    }

    public void tempToggle(boolean toggle) {
        activity.runOnUiThread(() -> {
            rootView.setVisibility(toggle ? View.VISIBLE : View.GONE);
        });
    }

    public void hide() {
        activity.runOnUiThread(() -> {
            if (rootView.getParent() != null) {
                ((ViewGroup) rootView.getParent()).removeView(rootView);
            }
        });
    }

    private void showColoredNotification(String prefix, String suffix, boolean isOn) {
        // TODO: хуйня
    }

    private void hideNotification() {
        // TODO: хуйня
    }

    public void updateSpeed(int speed) {
        activity.runOnUiThread(() -> {
            speedText.setText(String.valueOf(speed));
            if (speedLine != null) {
                speedLine.setProgress(Math.min(speed, 300));
            }
        });
    }

    public void update(int fuel, int hp, int engine, int light, int lock) {
        int hpPercent = Math.min(hp / 10, 100);
        int fuelLiters = fuel / 10;
        int fuelPercent = Math.min(fuel / 10, 100);

        activity.runOnUiThread(() -> {
            if (speedEngine != null) {
                speedEngine.setImageResource(engine == 1 ? R.drawable.ic_engine_on : R.drawable.ic_engine_off);
            }

            if (speedLights != null) {
                speedLights.setImageResource(light == 1 ? R.drawable.ic_lights_on : R.drawable.ic_lights_off);
            }

            if (speedDoors != null) {
                speedDoors.setImageResource(lock == 1 ? R.drawable.ic_doors_locked : R.drawable.ic_doors_unlocked);
            }

            if (fuelLine != null) {
                fuelLine.setProgress(fuelPercent);
            }
            if (fuelText != null) {
                fuelText.setText(String.valueOf(fuelLiters));
            }
        });
    }
}