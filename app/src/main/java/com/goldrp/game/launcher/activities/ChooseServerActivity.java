package com.goldrp.game.launcher.activities;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.ObjectAnimator;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.widget.ImageView;

import androidx.constraintlayout.widget.ConstraintLayout;

import com.goldrp.game.R;
import com.goldrp.game.core.Samp;
import com.goldrp.game.launcher.common.BaseActivity;
import com.goldrp.game.launcher.web.LauncherApi;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class ChooseServerActivity extends BaseActivity {

    private ImageView playCrimeBtn;
    private ImageView playGoldBtn;
    private ImageView playRublBtn;
    private ConstraintLayout backBtn;
    private Handler handler = new Handler(Looper.getMainLooper());
    private boolean isGameLaunched = false;

    private JSONArray serversArray;
    private String selectedServerIp;
    private int selectedServerPort;

    private ExecutorService executorService = Executors.newSingleThreadExecutor();
    private File sampSettingsFile;

    private static final int ANIMATION_DURATION = 100;
    private static final float SCALE_NORMAL = 1.0f;
    private static final float SCALE_PRESSED = 0.92f;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.launcher_choose_server_activity);

        File externalDir = getExternalFilesDir(null);
        if (externalDir != null) {
            sampSettingsFile = new File(externalDir, "SAMP/settings.ini");

            File sampDir = new File(externalDir, "SAMP");
            if (!sampDir.exists()) {
                sampDir.mkdirs();
            }
        }

        initViews();
        setupClickListeners();

        loadServerInfoFromUrl(LauncherApi.getServerInfo());
    }

    private void loadServerInfoFromUrl(String urlString) {
        executorService.execute(new Runnable() {
            @Override
            public void run() {
                try {
                    URL url = new URL(urlString);
                    HttpURLConnection connection = (HttpURLConnection) url.openConnection();
                    connection.setRequestMethod("GET");
                    connection.setConnectTimeout(5000);
                    connection.setReadTimeout(5000);

                    int responseCode = connection.getResponseCode();
                    if (responseCode == HttpURLConnection.HTTP_OK) {
                        InputStream inputStream = connection.getInputStream();
                        BufferedReader reader = new BufferedReader(new InputStreamReader(inputStream));
                        StringBuilder response = new StringBuilder();
                        String line;

                        while ((line = reader.readLine()) != null) {
                            response.append(line);
                        }

                        reader.close();
                        inputStream.close();

                        serversArray = new JSONArray(response.toString());
                    }

                    connection.disconnect();

                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        });
    }

    private void initViews() {
        playCrimeBtn = findViewById(R.id.play_crime_btn);
        playGoldBtn = findViewById(R.id.play_gold_btn);
        playRublBtn = findViewById(R.id.play_rubl_btn);
        backBtn = findViewById(R.id.back_btn);
    }

    /**
     * Применяет анимацию нажатия к кнопке
     */
    private void applyButtonAnimation(View button, final Runnable onClickAction) {
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(final View v) {
                v.animate()
                        .scaleX(SCALE_PRESSED)
                        .scaleY(SCALE_PRESSED)
                        .setDuration(ANIMATION_DURATION)
                        .withEndAction(new Runnable() {
                            @Override
                            public void run() {
                                v.animate()
                                        .scaleX(SCALE_NORMAL)
                                        .scaleY(SCALE_NORMAL)
                                        .setDuration(ANIMATION_DURATION)
                                        .withEndAction(new Runnable() {
                                            @Override
                                            public void run() {
                                                if (onClickAction != null) {
                                                    onClickAction.run();
                                                }
                                            }
                                        })
                                        .start();
                            }
                        })
                        .start();
            }
        });
    }

    /**
     * Альтернативный вариант анимации с эффектом пульсации
     */
    private void applyPulseAnimation(View button, final Runnable onClickAction) {
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(final View v) {
                v.animate()
                        .scaleX(SCALE_PRESSED)
                        .scaleY(SCALE_PRESSED)
                        .setDuration(ANIMATION_DURATION)
                        .withEndAction(new Runnable() {
                            @Override
                            public void run() {
                                v.animate()
                                        .scaleX(SCALE_NORMAL)
                                        .scaleY(SCALE_NORMAL)
                                        .setDuration(ANIMATION_DURATION)
                                        .withEndAction(new Runnable() {
                                            @Override
                                            public void run() {
                                                if (onClickAction != null) {
                                                    onClickAction.run();
                                                }
                                            }
                                        })
                                        .start();
                            }
                        })
                        .start();
            }
        });
    }

    /**
     * Анимация с помощью ObjectAnimator (более плавная)
     */
    private void applyObjectAnimatorAnimation(View button, final Runnable onClickAction) {
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                ObjectAnimator scaleDownX = ObjectAnimator.ofFloat(v, "scaleX", SCALE_NORMAL, SCALE_PRESSED);
                ObjectAnimator scaleDownY = ObjectAnimator.ofFloat(v, "scaleY", SCALE_NORMAL, SCALE_PRESSED);
                scaleDownX.setDuration(ANIMATION_DURATION);
                scaleDownY.setDuration(ANIMATION_DURATION);

                scaleDownX.addListener(new AnimatorListenerAdapter() {
                    @Override
                    public void onAnimationEnd(Animator animation) {
                        ObjectAnimator scaleUpX = ObjectAnimator.ofFloat(v, "scaleX", SCALE_PRESSED, SCALE_NORMAL);
                        ObjectAnimator scaleUpY = ObjectAnimator.ofFloat(v, "scaleY", SCALE_PRESSED, SCALE_NORMAL);
                        scaleUpX.setDuration(ANIMATION_DURATION);
                        scaleUpY.setDuration(ANIMATION_DURATION);

                        scaleUpX.addListener(new AnimatorListenerAdapter() {
                            @Override
                            public void onAnimationEnd(Animator animation) {
                                if (onClickAction != null) {
                                    onClickAction.run();
                                }
                            }
                        });

                        scaleUpX.start();
                        scaleUpY.start();
                    }
                });

                scaleDownX.start();
                scaleDownY.start();
            }
        });
    }

    private void setupClickListeners() {
        applyButtonAnimation(playCrimeBtn, new Runnable() {
            @Override
            public void run() {
                if (serversArray != null && serversArray.length() > 0) {
                    try {
                        JSONObject server = serversArray.getJSONObject(0);
                        selectedServerIp = server.getString("ip");
                        selectedServerPort = server.getInt("port");
                        saveServerToIni(selectedServerIp, selectedServerPort);
                        launchGame();
                    } catch (JSONException e) {
                        e.printStackTrace();
                    }
                }
            }
        });

        applyPulseAnimation(playGoldBtn, new Runnable() {
            @Override
            public void run() {
                if (serversArray != null && serversArray.length() > 1) {
                    try {
                        JSONObject server = serversArray.getJSONObject(1);
                        selectedServerIp = server.getString("ip");
                        selectedServerPort = server.getInt("port");
                        saveServerToIni(selectedServerIp, selectedServerPort);
                        launchGame();
                    } catch (JSONException e) {
                        e.printStackTrace();
                    }
                }
            }
        });

        applyObjectAnimatorAnimation(playRublBtn, new Runnable() {
            @Override
            public void run() {
                if (serversArray != null && serversArray.length() > 2) {
                    try {
                        JSONObject server = serversArray.getJSONObject(2);
                        selectedServerIp = server.getString("ip");
                        selectedServerPort = server.getInt("port");
                        saveServerToIni(selectedServerIp, selectedServerPort);
                        launchGame();
                    } catch (JSONException e) {
                        e.printStackTrace();
                    }
                }
            }
        });

        applyButtonAnimation(backBtn, new Runnable() {
            @Override
            public void run() {
                finish();
            }
        });
    }

    private void saveServerToIni(String ip, int port) {
        try {
            if (sampSettingsFile == null) return;

            StringBuilder content = new StringBuilder();
            boolean clientSectionExists = false;
            boolean hostPortAdded = false;

            if (sampSettingsFile.exists()) {
                BufferedReader reader = new BufferedReader(new FileReader(sampSettingsFile));
                String line;
                boolean inClientSection = false;

                while ((line = reader.readLine()) != null) {
                    if (line.trim().equals("[client]")) {
                        inClientSection = true;
                        clientSectionExists = true;
                        content.append(line).append("\n");
                    } else if (inClientSection) {
                        if (line.trim().startsWith("host=") || line.trim().startsWith("port=") ||
                                line.trim().startsWith("host =") || line.trim().startsWith("port =")) {
                            continue;
                        }
                        else if (line.trim().startsWith("[")) {
                            if (!hostPortAdded) {
                                content.append("host = ").append(ip).append("\n");
                                content.append("port = ").append(port).append("\n");
                                hostPortAdded = true;
                            }
                            inClientSection = false;
                            content.append(line).append("\n");
                        } else {
                            content.append(line).append("\n");
                        }
                    } else {
                        content.append(line).append("\n");
                    }
                }
                reader.close();

                if (inClientSection && !hostPortAdded) {
                    content.append("host = ").append(ip).append("\n");
                    content.append("port = ").append(port).append("\n");
                }
            }

            if (!clientSectionExists) {
                content.append("[client]\n");
                content.append("host = ").append(ip).append("\n");
                content.append("port = ").append(port).append("\n");
            } else if (!hostPortAdded) {
                String currentContent = content.toString();
                int lastClientIndex = currentContent.lastIndexOf("[client]");
                if (lastClientIndex >= 0) {
                    int nextSectionIndex = currentContent.indexOf("[", lastClientIndex + 8);
                    if (nextSectionIndex >= 0) {
                        content = new StringBuilder();
                        content.append(currentContent.substring(0, nextSectionIndex));
                        content.append("host = ").append(ip).append("\n");
                        content.append("port = ").append(port).append("\n");
                        content.append(currentContent.substring(nextSectionIndex));
                    } else {
                        content.append("host = ").append(ip).append("\n");
                        content.append("port = ").append(port).append("\n");
                    }
                }
            }

            FileOutputStream fos = new FileOutputStream(sampSettingsFile);
            OutputStreamWriter writer = new OutputStreamWriter(fos);
            writer.write(content.toString());
            writer.close();
            fos.close();

        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void removeServerFromIni() {
        try {
            if (sampSettingsFile == null || !sampSettingsFile.exists()) {
                return;
            }

            StringBuilder content = new StringBuilder();
            BufferedReader reader = new BufferedReader(new FileReader(sampSettingsFile));
            String line;
            boolean inClientSection = false;

            while ((line = reader.readLine()) != null) {
                if (line.trim().equals("[client]")) {
                    inClientSection = true;
                    content.append(line).append("\n");
                } else if (inClientSection) {
                    if (line.trim().startsWith("host=") || line.trim().startsWith("port=") ||
                            line.trim().startsWith("host =") || line.trim().startsWith("port =")) {
                        continue;
                    } else if (line.trim().startsWith("[")) {
                        inClientSection = false;
                        content.append(line).append("\n");
                    } else {
                        content.append(line).append("\n");
                    }
                } else {
                    content.append(line).append("\n");
                }
            }
            reader.close();

            FileOutputStream fos = new FileOutputStream(sampSettingsFile);
            OutputStreamWriter writer = new OutputStreamWriter(fos);
            writer.write(content.toString());
            writer.close();
            fos.close();

        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void launchGame() {
        if (isGameLaunched) {
            return;
        }
        isGameLaunched = true;

        Intent gameIntent = new Intent(ChooseServerActivity.this, Samp.class);
        startActivity(gameIntent);
    }

    @Override
    protected void onResume() {
        super.onResume();
        removeServerFromIni();
        isGameLaunched = false;
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        handler.removeCallbacksAndMessages(null);
        executorService.shutdown();
    }
}