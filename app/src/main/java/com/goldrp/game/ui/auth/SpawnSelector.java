package com.goldrp.game.ui.auth;

import android.app.Activity;
import android.widget.ImageView;

import androidx.appcompat.app.AppCompatActivity;
import androidx.constraintlayout.widget.ConstraintLayout;

import com.goldrp.game.R;
import com.goldrp.game.core.Samp;
import com.goldrp.game.launcher.util.Util;

public class SpawnSelector extends AppCompatActivity {
    public Activity activity;

    public ConstraintLayout selectbr;
    public ConstraintLayout logoutItem, stationItem, fractionItem, homeItem, familyItem;
    public ImageView logoutImg, stationImg, fractionImg, homeImg, familyImg;
    public static int type = 0;

    public SpawnSelector(Activity aactivity) {
        activity = aactivity;
        selectbr = aactivity.findViewById(R.id.selectSpawnMainLayout);

        if (selectbr == null) {
            return;
        }

        logoutItem = aactivity.findViewById(R.id.selectSpawnItemLogout);
        stationItem = aactivity.findViewById(R.id.selectSpawnItemStation);
        fractionItem = aactivity.findViewById(R.id.selectSpawnItemFraction);
        homeItem = aactivity.findViewById(R.id.selectSpawnItemHome);
        familyItem = aactivity.findViewById(R.id.selectSpawnItemFamily);

        logoutImg = aactivity.findViewById(R.id.selectSpawnItemLogoutImg);
        stationImg = aactivity.findViewById(R.id.selectSpawnItemStationImg);
        fractionImg = aactivity.findViewById(R.id.selectSpawnItemFractionImg);
        homeImg = aactivity.findViewById(R.id.selectSpawnItemHomeImg);
        familyImg = aactivity.findViewById(R.id.selectSpawnItemFamilyImg);

        if (selectbr != null) {
            Util.HideLayout(selectbr, false);
        }

        if (logoutItem != null) {
            logoutItem.setOnClickListener(view -> {
                type = 1;
                resetSelection();
                if (logoutImg != null) {
                    logoutImg.setImageTintList(android.content.res.ColorStateList.valueOf(activity.getColor(R.color.bkuzn)));
                }
                selectSpawn();
            });
        }

        if (stationItem != null) {
            stationItem.setOnClickListener(view -> {
                type = 2;
                resetSelection();
                if (stationImg != null) {
                    stationImg.setImageTintList(android.content.res.ColorStateList.valueOf(activity.getColor(R.color.bkuzn)));
                }
                selectSpawn();
            });
        }

        if (fractionItem != null) {
            fractionItem.setOnClickListener(view -> {
                type = 3;
                resetSelection();
                if (fractionImg != null) {
                    fractionImg.setImageTintList(android.content.res.ColorStateList.valueOf(activity.getColor(R.color.bkuzn)));
                }
                selectSpawn();
            });
        }

        if (homeItem != null) {
            homeItem.setOnClickListener(view -> {
                type = 4;
                resetSelection();
                if (homeImg != null) {
                    homeImg.setImageTintList(android.content.res.ColorStateList.valueOf(activity.getColor(R.color.bkuzn)));
                }
                selectSpawn();
            });
        }

        if (familyItem != null) {
            familyItem.setOnClickListener(view -> {
                type = 5;
                resetSelection();
                if (familyImg != null) {
                    familyImg.setImageTintList(android.content.res.ColorStateList.valueOf(activity.getColor(R.color.bkuzn)));
                }
                selectSpawn();
            });
        }
    }

    private void resetSelection() {
        if (logoutImg != null) {
            logoutImg.setImageTintList(android.content.res.ColorStateList.valueOf(activity.getColor(android.R.color.white)));
        }
        if (stationImg != null) {
            stationImg.setImageTintList(android.content.res.ColorStateList.valueOf(activity.getColor(android.R.color.white)));
        }
        if (fractionImg != null) {
            fractionImg.setImageTintList(android.content.res.ColorStateList.valueOf(activity.getColor(android.R.color.white)));
        }
        if (homeImg != null) {
            homeImg.setImageTintList(android.content.res.ColorStateList.valueOf(activity.getColor(android.R.color.white)));
        }
        if (familyImg != null) {
            familyImg.setImageTintList(android.content.res.ColorStateList.valueOf(activity.getColor(android.R.color.white)));
        }
    }

    private void selectSpawn() {
        if (type != 0) {
            Samp.getInstance().sendSpawnClick(type);
            hide();
        }
    }

    public void showselect() {
        if (selectbr != null) {
            Util.ShowLayout(selectbr, true);
        }
    }

    public void hide() {
        if (selectbr != null) {
            Util.HideLayout(selectbr, true);
        }
    }
}