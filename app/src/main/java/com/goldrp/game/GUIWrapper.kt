package com.goldrp.game

import org.json.JSONObject

interface GUIWrapper {
    fun onShow(data: JSONObject?)
    fun onClose()
    fun receiveUIpacket(data: JSONObject?)
}
