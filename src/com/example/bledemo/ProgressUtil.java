package com.example.bledemo;


import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.view.LayoutInflater;
import android.view.View;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;


public final class ProgressUtil {
	private static final String TAG = "ProgressUtil";
	private static Dialog progressDialog = null;
	
	private ProgressUtil() {}
	
	public static void show(Context context, String message) {
		if(progressDialog == null){
			LayoutInflater inflater = LayoutInflater.from(context);  
			View v = inflater.inflate(R.layout.dialog_loading, null);// �õ�����view  
			LinearLayout layout = (LinearLayout) v.findViewById(R.id.dialog_view);// ���ز���  
			// main.xml�е�ImageView  
			ImageView spaceshipImage = (ImageView) v.findViewById(R.id.img_loading);  
			TextView tipTextView = (TextView) v.findViewById(R.id.tipTextView);// ��ʾ����
			// ���ض���  
			Animation hyperspaceJumpAnimation = AnimationUtils.loadAnimation(  
					context, R.anim.loading_animation);  
			// ʹ��ImageView��ʾ����  
			spaceshipImage.startAnimation(hyperspaceJumpAnimation);  
			tipTextView.setText(message);// ���ü�����Ϣ  
			progressDialog = new Dialog(context, R.style.loading_dialog);// �����Զ�����ʽdialog  
			
			progressDialog.setCancelable(false);// �������á����ؼ���ȡ��  
			progressDialog.setCanceledOnTouchOutside(false);
			progressDialog.setContentView(layout, new LinearLayout.LayoutParams(  
					LinearLayout.LayoutParams.MATCH_PARENT,  
					LinearLayout.LayoutParams.MATCH_PARENT));// ���ò���  
			progressDialog.show();
			return;
		}
		if(progressDialog.isShowing()){
			update(message);
		}else{
			update(message);
			progressDialog.show();
		}
	}
	
	public static void update(String message){
		if(progressDialog == null) return;
		View view = progressDialog.getWindow().getDecorView();
		TextView tvUpdate = (TextView) view.findViewById(R.id.tipTextView);
		if (tvUpdate != null) {
			tvUpdate.setText(message);
		}
	}
	
	public static void hide() {
		if (progressDialog != null) {
			if (progressDialog.isShowing()) {
				progressDialog.dismiss();
			}
			progressDialog = null;
		}
	}
	
	public static void setOnKeyListener(DialogInterface.OnKeyListener listener){
		if (progressDialog != null) {
			progressDialog.setOnKeyListener(listener);
		}
	}
}
