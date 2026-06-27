package com.goldrp.game.common.views;
public class Preferences {
    // Для отправки и перехвата диалога
    public static int CurrentDialogId = 0;
    public static int ListitemToSend = 0;

    // ID Player
    public static int playerID = 0;

    // Snow
    public static int snow = 0;

    // Автовход и рег/логин
    public static String nickname = "";
    public static String password_login = "";

    public static String key_board = "";

    public static Boolean autologin = false;
    
    // дураки
    public static void setAll(int iCurrentDialogId, int iListitemToSend){
        setCurrentDialogId(iCurrentDialogId);
        setListitemToSend(iListitemToSend);
    }
   
    // не любят
    public static int GetCurrentDialogId() { return CurrentDialogId; }
    public static int setCurrentDialogId(int nCurrentDialogId) {
        return CurrentDialogId = nCurrentDialogId;
    }
    
    // гениальных
    public static int GetListitemToSend() { return ListitemToSend; }
    public static int setListitemToSend(int nListitemToSend) {
        return ListitemToSend = nListitemToSend;
    }

    public static String getKeyBoardText(){ return key_board; }
    public static String setKeyBoardText(String string){
        return key_board = string;
    }

    // TODO: дурачок :)
    public static String getNick(){ return nickname; }
    public static String setNick(String string){
        return nickname = string;
    }

    public static String getPassword(){ return password_login; }
    public static String setPassword(String string){
        return password_login = string;
    }

    public static int GetID() { return playerID; }
    public static int setUID(int id) {
        return playerID = id;
    }

    public static boolean GetAutoLoginStatus() { return autologin; }
    public static boolean setAL(boolean tf) {
        return autologin = tf;
    }

    public static int snowget() { return snow; }
    public static int setsnow(int ngId) {
        return snow = ngId;
    }

    public static Boolean getReverser2() {
        return true;
    }
}
