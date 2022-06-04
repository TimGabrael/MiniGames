package com.GabelStudio.MiniGames;

import android.annotation.SuppressLint;

import androidx.appcompat.app.AppCompatActivity;
import androidx.constraintlayout.widget.ConstraintLayout;

import android.app.ActionBar;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.text.Layout;
import android.util.Log;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowInsets;
import android.widget.Button;
import android.widget.GridLayout;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.Space;
import android.widget.TableLayout;
import android.widget.TableRow;
import android.widget.TextView;

import com.GabelStudio.MiniGames.databinding.ActivityLobbyBinding;


public class LobbyActivity extends AppCompatActivity {

    private ActivityLobbyBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        binding = ActivityLobbyBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());


        TableLayout lay = (TableLayout)findViewById(R.id.GameTable);

        int rowHeight = 400;

        TableRow first = new TableRow(this);
        first.setMinimumHeight(rowHeight);

        ImageButton imgBtn = new ImageButton(this);
        imgBtn.setImageResource(R.mipmap.uno_image_foreground);
        imgBtn.setMinimumHeight(rowHeight);
        imgBtn.setLayoutParams(new TableRow.LayoutParams(TableRow.LayoutParams.MATCH_PARENT, TableRow.LayoutParams.WRAP_CONTENT, 1.0f));
        first.addView(imgBtn);


        lay.addView(first);


        LinearLayout mainScrl = (LinearLayout)findViewById(R.id.playerLayout);
        if(mainScrl != null) {
            TextView tView = new TextView(this);
            tView.setText("TestString");
            tView.setGravity(Gravity.CENTER);
            mainScrl.addView(tView);
        }
        else
        {
            Log.e("TAG", "onCreate: IS NULL");
        }
    }
}