package com.example.bledemo;

import java.lang.Thread.UncaughtExceptionHandler;
import java.util.ArrayList;
import android.app.Activity;
import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends Activity implements OnClickListener, BluetoothCallBack, UncaughtExceptionHandler {

	BluetoothUtil bluetoothUtil;
	Context context;

	// 蓝牙打开图片
	private static final int BLE_OPEN = 1;

	// 蓝牙关闭图片
	private static final int BLE_CLOSE = 2;

	// 信息
	private static final int TV_SHOW = 3;

	// 扫描到的设备
	private static final int SCAN_DEVICE = 4;

	ImageView iv_openble;

	Button btn_start, btn_uncon;

	ListView lv_ble;

	TextView tv_youself, tv_show;
	ArrayList<BluetoothDevice> list_device = new ArrayList<BluetoothDevice>(); ;

	BluetoothDevice device;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		setContentView(R.layout.activity_main);
		Thread.setDefaultUncaughtExceptionHandler(this);
		initView();
	}

	// View的初始化
	public void initView() {
		context = this;
		bluetoothUtil = new BluetoothUtil(context, this);
		bluetoothUtil.registerBoard();
		iv_openble = (ImageView) findViewById(R.id.iv_openble);
		tv_youself = (TextView) findViewById(R.id.tv_youself);
		tv_show = (TextView) findViewById(R.id.tv_show);
		btn_start = (Button) findViewById(R.id.btn_start);
		btn_uncon = (Button) findViewById(R.id.btn_uncon);
		lv_ble = (ListView) findViewById(R.id.lv_ble);
		tv_youself.setText(bluetoothUtil.getSelfName());
		iv_openble.setOnClickListener(this);
		btn_start.setOnClickListener(this);
		btn_uncon.setOnClickListener(this);
	}

	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.iv_openble:
				if (bluetoothUtil.isOpenBle) {
					bluetoothUtil.closeBle();
					handler.sendEmptyMessage(BLE_CLOSE);
				}else {
					bluetoothUtil.openBle();
					handler.sendEmptyMessage(BLE_OPEN);
				}
			break;

		case R.id.btn_start:
			if (bluetoothUtil.isOpenBle) {
				bluetoothUtil.startScan(5);
			} else {
				Toast.makeText(context, "请先打开蓝牙", Toast.LENGTH_SHORT).show();
			}
			break;

		case R.id.btn_uncon:
			if (bluetoothUtil.isConn==false) {
				bluetoothUtil.stopConn();
				myHandMessage("无");
			}else{
				Toast.makeText(this, "还未连接", Toast.LENGTH_SHORT).show();
			}
			
			break;

		default:
			break;
		}
	}

	private Handler handler = new Handler() {
		public void handleMessage(android.os.Message msg) {
			switch (msg.what) {
			case BLE_OPEN:
				iv_openble.setImageResource(R.drawable.bleopen);
				break;

			case BLE_CLOSE:
				iv_openble.setImageResource(R.drawable.bleclose);
				break;

			case TV_SHOW:
				String info = (String) msg.obj;
				tv_show.setText(info);
				break;

			case SCAN_DEVICE:
				list_device = (ArrayList<BluetoothDevice>) msg.obj;
				String[] items =new String[list_device.size()]; // 设备名称的集合列表

				for (int i = 0; i < list_device.size(); i++) {
					items[i] = list_device.get(i).getName();
				}
				ArrayAdapter<String> adapter = new ArrayAdapter<String>(context, android.R.layout.simple_list_item_1,
						items);
				lv_ble.setAdapter(adapter);
				lv_ble.setOnItemClickListener(new OnItemClickListener() {

					@Override
					public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
						if (bluetoothUtil.isOpenBle) {
							if (bluetoothUtil.isConn) {
								Toast.makeText(context, "请先断开连接", Toast.LENGTH_SHORT).show();
							}else{
								BluetoothDevice device = list_device.get(position);
								bluetoothUtil.startConn(device);
							}
						}else {
							Toast.makeText(context, "请先打开蓝牙", Toast.LENGTH_SHORT).show();
						}
					}
				});
				break;
	
			default:
				break;
			}
		};
	};

	@Override
	public void uncaughtException(Thread thread, Throwable ex) {
		Log.i("MainActivity", "uncaughtException  " + ex);
	}

	public void myHandMessage(String str) {
		Message message = new Message();
		message.what = TV_SHOW;
		message.obj = str;
		handler.sendMessage(message);
	}
	
	@Override
	public void bluOpen() {
		myHandMessage("蓝牙打开");
		handler.sendEmptyMessage(BLE_OPEN);
	}

	@Override
	public void bluClose() {
		myHandMessage("蓝牙断开");
		handler.sendEmptyMessage(BLE_CLOSE);
	}
	

	@Override
	public void startScan() {
		ProgressUtil.show(context, "正在搜索...");
	}

	@Override
	public void findDevice(ArrayList<BluetoothDevice> list) {
		if (list.size() == 0) {
			myHandMessage("未查找到设备");
		}else{
			Message message = new Message();
			message.what = SCAN_DEVICE;
			message.obj = list;
			handler.sendMessage(message);
		}
	}

	@Override
	public void finishFind(ArrayList<BluetoothDevice> list) {
		ProgressUtil.hide();
	}

	@Override
	public void connectionSucc(BluetoothDevice device) {
		ProgressUtil.hide();
		String name = device.getName();
		myHandMessage(name);
	}

	@Override
	public void connectionFail() {
		ProgressUtil.hide();
		myHandMessage("连接失败");
	}

	@Override
	public void startConn() {
		ProgressUtil.show(context,"正在连接设备...");
	}

	@Override
	public void BondFail() {
		myHandMessage("绑定失败");
		ProgressUtil.hide();
	}

	@Override
	public void BondSuccess() {
		myHandMessage("绑定成功");
	}

	@Override
	public void Bonding() {
		myHandMessage("正在绑定");
	}

}
