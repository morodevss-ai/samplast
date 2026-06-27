package com.goldrp.game;

import android.app.Activity;
import android.view.ViewGroup;

import com.goldrp.game.GUIWrapper;
import com.goldrp.game.ui.loading.LoadingScreen;

import org.json.JSONObject;
import java.util.HashMap;

public final class GUIManager {
    private static final HashMap<Integer, GUIWrapper> activeSystems = new HashMap<>();
    private static Activity activity;
    private static ViewGroup root;
    private static ViewGroup mainMenuRoot;
    private GUIManager() {}

    public static void setup(Activity activity, ViewGroup root, ViewGroup mainMenuRoot) {
        GUIManager.activity = activity;
        GUIManager.root = root;
        GUIManager.mainMenuRoot = mainMenuRoot;
    }
    public static void receiveUIpacket(int id, JSONObject data) {
        activity.runOnUiThread(() -> {
            if (id >= 256 || id < 0) {
                return;
            }

            if (data.optInt("o") == 2) {
                showGUI(id, data);
                return;
            }
            if (data.optInt("o") == 1) {
                closeGUI(id);
                return;
            }

            if (activeSystems.get(id) == null) {
                return;
            }

            GUIWrapper gui = activeSystems.get(id);
            if (gui != null) {
                gui.receiveUIpacket(data);
            }
        });
    }

    private static void showGUI(int id, JSONObject data) {
        GUIWrapper system = activeSystems.get(id);
        if (system == null) {
            system = createSystemGUIById(id);
            if (system == null) {
                System.err.println("Unknown system id: " + id);
                return;
            }
            activeSystems.remove(id, system);
            activeSystems.put(id, system);
        } else {
            system.onClose();
        }
        system.onShow(data);
    }

    private static void closeGUI(int id) {
        GUIWrapper system = activeSystems.remove(id);
        if (system != null) {
            system.onClose();
        } else {
            System.err.println("No system to close with id: " + id);
        }
    }

    private static GUIWrapper createSystemGUIById(int id) {
        switch (id) {
            case 1:
                // return LoadingScreen.newInstance(activity, root);
            default:
                return null;
        }
    }
}