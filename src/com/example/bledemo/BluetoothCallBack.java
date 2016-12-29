package com.example.bledemo;

import java.util.ArrayList;
import java.util.Set;

import android.bluetooth.BluetoothDevice;

public interface BluetoothCallBack {
	void startScan();//¿ªÊ¼É¨Ãè
	void findDevice(ArrayList<BluetoothDevice> list);
	void finishFind(ArrayList<BluetoothDevice> list);
	
	void bluOpen();
	void bluClose();
	
	void connectionSucc(BluetoothDevice device);
	void connectionFail();
	void startConn();
	
	void BondFail();
	void BondSuccess();
	void Bonding();
}
