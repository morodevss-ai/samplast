package com.goldrp.game.ui.reg;

import android.app.Activity;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.View;
import android.view.animation.AnimationUtils;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.constraintlayout.widget.ConstraintLayout;

import com.goldrp.game.R;
import com.goldrp.game.core.Samp;
import com.goldrp.game.common.views.Preferences;
import com.goldrp.game.ui.dialog.DialogManager;
import com.google.android.material.button.MaterialButton;
import com.google.android.material.switchmaterial.SwitchMaterial;

import java.io.UnsupportedEncodingException;

public class Reg {
    public Activity activity;

    public FrameLayout RegistrationLayout;
    public ConstraintLayout registrationLayout;
    public ConstraintLayout sexLayout;
    public FrameLayout authorizationLayout;

    public TextView regNick;
    public EditText regMail;
    public EditText regPassword;
    public EditText regPasswordTwo;
    public MaterialButton regPlay;
    public ConstraintLayout regBack;
    public ConstraintLayout regInfoContainer;
    public TextView regInfoTitle;
    public TextView regInfo;

    public ImageView choosesexMale;
    public ImageView choosesexFemale;
    public MaterialButton choosesexBtn;

    public ConstraintLayout authRight2;
    public EditText authPassword;
    public MaterialButton authPlay;
    public TextView authNick;
    public TextView recoveryPassword;
    public ConstraintLayout authBack;
    public SwitchMaterial switcherAutologin;
    public ConstraintLayout autologinMainHelp;
    public MaterialButton autologinHelpClose;
    public ImageView autoLoginInfo;
    public ConstraintLayout authInfoContainer;
    public TextView authInfoTitle;
    public TextView authInfo;

    static Boolean isTurned = false;
    static Boolean isReg = false;

    public Reg(Activity aactivity) {
        activity = aactivity;

        RegistrationLayout = aactivity.findViewById(R.id.br_registration_layout);
        registrationLayout = aactivity.findViewById(R.id.registration_layout);
        sexLayout = aactivity.findViewById(R.id.sex_layout);
        authorizationLayout = aactivity.findViewById(R.id.authorizationLayout);

        regNick = aactivity.findViewById(R.id.reg_nick);
        regMail = aactivity.findViewById(R.id.reg_mail);
        regPassword = aactivity.findViewById(R.id.reg_password);
        regPasswordTwo = aactivity.findViewById(R.id.reg_passwordtwo);
        regPlay = aactivity.findViewById(R.id.reg_play);
        regBack = aactivity.findViewById(R.id.reg_back);
        regInfoContainer = aactivity.findViewById(R.id.regInfoContainer);
        regInfoTitle = aactivity.findViewById(R.id.reg_infotitle);
        regInfo = aactivity.findViewById(R.id.reg_info);

        choosesexMale = aactivity.findViewById(R.id.choosesex_male);
        choosesexFemale = aactivity.findViewById(R.id.choosesex_female);
        choosesexBtn = aactivity.findViewById(R.id.choosesex_btn);

        authPassword = aactivity.findViewById(R.id.authPassword);
        authPlay = aactivity.findViewById(R.id.authPlay);
        authNick = aactivity.findViewById(R.id.authNick);
        recoveryPassword = aactivity.findViewById(R.id.recoveryPassword);
        authBack = aactivity.findViewById(R.id.authBack);
        switcherAutologin = aactivity.findViewById(R.id.switcherAutologin);
        autologinMainHelp = aactivity.findViewById(R.id.autologinMainHelp);
        autologinHelpClose = aactivity.findViewById(R.id.autologinHelpClose);
        autoLoginInfo = aactivity.findViewById(R.id.autoLoginInfo);
        authInfoContainer = aactivity.findViewById(R.id.authInfoContainer);
        authInfoTitle = aactivity.findViewById(R.id.authInfoTitle);
        authInfo = aactivity.findViewById(R.id.authInfo);

        setupListeners();
        updateNickname();
    }

    private void setupListeners() {
        if (switcherAutologin != null) {
            switcherAutologin.setOnCheckedChangeListener((buttonView, isChecked) -> {
                if (isChecked) {
                    isTurned = true;
                    Preferences.setAL(true);
                    Samp.getInstance().showNotification(2, "Недоступно", 4, "", "");
                } else {
                    isTurned = false;
                    Preferences.setAL(false);
                }
            });
        }

        if (autoLoginInfo != null) {
            autoLoginInfo.setOnClickListener(v -> {
                if (autologinMainHelp != null) {
                    autologinMainHelp.setVisibility(View.VISIBLE);
                }
            });
        }

        if (autologinHelpClose != null) {
            autologinHelpClose.setOnClickListener(v -> {
                if (autologinMainHelp != null) {
                    autologinMainHelp.setVisibility(View.GONE);
                }
            });
        }

        if (authPassword != null) {
            authPassword.addTextChangedListener(new TextWatcher() {
                @Override
                public void afterTextChanged(Editable s) {}

                @Override
                public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

                @Override
                public void onTextChanged(CharSequence s, int start, int before, int count) {
                    try {
                        Preferences.setPassword(String.valueOf(authPassword.getText()));
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                }
            });
        }

        if (authBack != null) {
            authBack.setOnClickListener(view -> {
                view.startAnimation(AnimationUtils.loadAnimation(activity, R.anim.button_click));
            });
        }

        if (authPlay != null) {
            authPlay.setOnClickListener(view -> {
                view.startAnimation(AnimationUtils.loadAnimation(activity, R.anim.button_click));

                DialogManager dialogManager = DialogManager.getInstance();
                if (dialogManager == null) {
                    Samp.getInstance().showNotification(2, "Ошибка системы", 5, "", "");
                    return;
                }

                try {
                    if (checkValidAuth()) {
                        dialogManager.sendDialogResponse(
                                1,
                                1,
                                Preferences.GetListitemToSend(),
                                authPassword.getText().toString().getBytes("windows-1251")
                        );
                        HideAuth();
                    }
                } catch (UnsupportedEncodingException e) {
                    Samp.getInstance().showNotification(2, "Произошла неизвестная ошибка", 5, "", "");
                    e.printStackTrace();
                }
            });
        }

        if (regPassword != null) {
            regPassword.addTextChangedListener(new TextWatcher() {
                @Override
                public void afterTextChanged(Editable s) {}

                @Override
                public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

                @Override
                public void onTextChanged(CharSequence s, int start, int before, int count) {
                    try {
                        Preferences.setPassword(String.valueOf(regPassword.getText()));
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                }
            });
        }

        if (regPlay != null) {
            regPlay.setOnClickListener(view -> {
                view.startAnimation(AnimationUtils.loadAnimation(activity, R.anim.button_click));

                DialogManager dialogManager = DialogManager.getInstance();
                if (dialogManager == null) {
                    Samp.getInstance().showNotification(2, "Ошибка системы", 5, "", "");
                    return;
                }

                try {
                    if (checkValidReg() && regPassword.getText().toString().equals(regPasswordTwo.getText().toString())) {
                        dialogManager.sendDialogResponse(
                                1,
                                2,
                                Preferences.GetListitemToSend(),
                                regPassword.getText().toString().getBytes("windows-1251")
                        );

                        HideReg();
                        ShowSexSelection();

                    } else {
                        Samp.getInstance().showNotification(2, "Пароли не совпадают", 5, "", "");
                    }
                } catch (UnsupportedEncodingException e) {
                    Samp.getInstance().showNotification(2, "Произошла неизвестная ошибка", 5, "", "");
                    e.printStackTrace();
                }
            });
        }

        if (choosesexMale != null) {
            choosesexMale.setOnClickListener(v -> {
                v.startAnimation(AnimationUtils.loadAnimation(activity, R.anim.button_click));

                DialogManager dialogManager = DialogManager.getInstance();
                if (dialogManager == null) {
                    Samp.getInstance().showNotification(2, "Ошибка системы", 5, "", "");
                    return;
                }

                try {
                    dialogManager.sendDialogResponse(
                            1,
                            2,
                            Preferences.GetListitemToSend(),
                            "1".getBytes("windows-1251")
                    );
                } catch (UnsupportedEncodingException e) {
                    e.printStackTrace();
                }
                HideSexSelection();
                Samp.getInstance().showNotification(3, "Добро пожаловать!", 4, "", "");
            });
        }

        if (choosesexFemale != null) {
            choosesexFemale.setOnClickListener(v -> {
                v.startAnimation(AnimationUtils.loadAnimation(activity, R.anim.button_click));

                DialogManager dialogManager = DialogManager.getInstance();
                if (dialogManager == null) {
                    Samp.getInstance().showNotification(2, "Ошибка системы", 5, "", "");
                    return;
                }

                try {
                    dialogManager.sendDialogResponse(
                            0,
                            2,
                            Preferences.GetListitemToSend(),
                            "0".getBytes("windows-1251")
                    );
                } catch (UnsupportedEncodingException e) {
                    e.printStackTrace();
                }
                HideSexSelection();
                Samp.getInstance().showNotification(3, "Добро пожаловать!", 4, "", "");
            });
        }

        if (choosesexBtn != null) {
            choosesexBtn.setOnClickListener(v -> {
                v.startAnimation(AnimationUtils.loadAnimation(activity, R.anim.button_click));

                DialogManager dialogManager = DialogManager.getInstance();
                if (dialogManager == null) {
                    Samp.getInstance().showNotification(2, "Ошибка системы", 5, "", "");
                    return;
                }

                try {
                    dialogManager.sendDialogResponse(
                            1,
                            2,
                            Preferences.GetListitemToSend(),
                            "1".getBytes("windows-1251")
                    );
                } catch (UnsupportedEncodingException e) {
                    e.printStackTrace();
                }
                HideSexSelection();
                Samp.getInstance().showNotification(3, "Добро пожаловать!", 4, "", "");
            });
        }

        if (recoveryPassword != null) {
            recoveryPassword.setOnClickListener(v -> {
                v.startAnimation(AnimationUtils.loadAnimation(activity, R.anim.button_click));
                Samp.getInstance().showNotification(2, "Функция временно недоступна", 4, "", "");
            });
        }
    }

    public void updateNickname() {
        String nick = Preferences.getNick();
        String id = String.valueOf(Preferences.GetID());
        String displayText = nick + " [" + id + "]";

        if (regNick != null) {
            regNick.setText(displayText);
        }

        if (authNick != null) {
            authNick.setText(displayText);
        }
    }

    public void ShowAuth() {
        if (authorizationLayout != null) {
            authorizationLayout.setVisibility(View.VISIBLE);
        }
        if (registrationLayout != null) {
            registrationLayout.setVisibility(View.GONE);
        }
        if (sexLayout != null) {
            sexLayout.setVisibility(View.GONE);
        }
        isReg = false;
    }

    public void HideAuth() {
        if (authorizationLayout != null) {
            authorizationLayout.setVisibility(View.GONE);
        }
    }

    public void ShowReg() {
        isReg = true;
        if (registrationLayout != null) {
            registrationLayout.setVisibility(View.VISIBLE);
        }
        if (authorizationLayout != null) {
            authorizationLayout.setVisibility(View.GONE);
        }
        if (sexLayout != null) {
            sexLayout.setVisibility(View.GONE);
        }
    }

    public void HideReg() {
        if (registrationLayout != null) {
            registrationLayout.setVisibility(View.GONE);
        }
    }

    public void ShowSexSelection() {
        if (sexLayout != null) {
            sexLayout.setVisibility(View.VISIBLE);
        }
        if (registrationLayout != null) {
            registrationLayout.setVisibility(View.GONE);
        }
    }

    public void HideSexSelection() {
        if (sexLayout != null) {
            sexLayout.setVisibility(View.GONE);
        }
    }

    public boolean checkValidAuth() {
        if (authPassword == null) return false;

        String password = authPassword.getText().toString();
        if (password.isEmpty()) {
            Samp.getInstance().showNotification(2, "Введите пароль", 5, "", "");
            return false;
        }
        if (password.length() < 6) {
            Samp.getInstance().showNotification(2, "Длина пароля должна быть не менее 6 символов", 5, "", "");
            return false;
        }
        return true;
    }

    public boolean checkValidReg() {
        if (regPassword == null || regPasswordTwo == null) return false;

        String password = regPassword.getText().toString();
        String passwordConfirm = regPasswordTwo.getText().toString();

        if (password.isEmpty()) {
            Samp.getInstance().showNotification(2, "Введите пароль", 5, "", "");
            return false;
        }
        if (password.length() < 6) {
            Samp.getInstance().showNotification(2, "Длина пароля должна быть не менее 6 символов", 5, "", "");
            return false;
        }
        if (passwordConfirm.isEmpty()) {
            Samp.getInstance().showNotification(2, "Повторите пароль", 5, "", "");
            return false;
        }
        return true;
    }
}