package com.example.bt_esp32;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.util.UUID;

public class MainActivity extends AppCompatActivity {

    private static final String TAG = "BiorreatorApp";
    private static final UUID MY_UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");
    private static final int REQUEST_ENABLE_BT = 1;
    private static final int REQUEST_CONNECT_DEVICE = 2;

    private BluetoothAdapter bluetoothAdapter;
    private BluetoothSocket bluetoothSocket = null;
    private ConnectedThread connectedThread;
    private boolean isConnected = false;

    // Componentes da UI
    private Button btnConectar, btnIniciarCiclo, btnAbrirMonitor;
    private EditText etTemperatura, etTempoCiclo, etVolumeRefil, etVolumeExtracao;
    private TextView tvStatusConexao;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        vincularComponentesUI();
        configurarListeners();
        verificarPermissoesBluetooth();

        bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        if (bluetoothAdapter == null) {
            Toast.makeText(this, "Bluetooth não suportado", Toast.LENGTH_LONG).show();
            finish();
        }
    }

    private void configurarListeners() {
        btnConectar.setOnClickListener(v -> {
            if (isConnected) {
                desconectar();
            } else {
                conectar();
            }
        });

        btnIniciarCiclo.setOnClickListener(v -> {
            // Coleta os dados dos campos de texto
            String temp = etTemperatura.getText().toString();
            String tempo = etTempoCiclo.getText().toString();
            String refil = etVolumeRefil.getText().toString();
            String extracao = etVolumeExtracao.getText().toString();

            // Monta o comando para o ESP32
            // Formato: "C;temp;tempo;refil;extracao;"
            String comando = "C;" + temp + ";" + tempo + ";" + refil + ";" + extracao + ";";

            if (connectedThread != null) {
                connectedThread.write(comando);
                Toast.makeText(this, "Ciclo iniciado!", Toast.LENGTH_SHORT).show();
                btnAbrirMonitor.performClick(); // Abre o monitor automaticamente
            }
        });

        btnAbrirMonitor.setOnClickListener(v -> {
            Intent intent = new Intent(this, MonitorActivity.class);
            startActivity(intent);
        });
    }

    private void conectar() {
        if (!bluetoothAdapter.isEnabled()) {
            Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
        } else {
            Intent intent = new Intent(this, DeviceListActivity.class);
            startActivityForResult(intent, REQUEST_CONNECT_DEVICE);
        }
    }

    private void desconectar() {
        try {
            isConnected = false;
            if (bluetoothSocket != null) {
                bluetoothSocket.close();
            }
            btnConectar.setText("Conectar ao Biorreator");
            tvStatusConexao.setText("Status: Desconectado");
            tvStatusConexao.setTextColor(Color.RED);
            btnIniciarCiclo.setEnabled(false);
            btnAbrirMonitor.setEnabled(false);
            Toast.makeText(this, "Desconectado", Toast.LENGTH_SHORT).show();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    @SuppressLint("MissingPermission")
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == REQUEST_CONNECT_DEVICE && resultCode == Activity.RESULT_OK) {
            String macAddress = data.getStringExtra(DeviceListActivity.EXTRA_DEVICE_ADDRESS);
            BluetoothDevice device = bluetoothAdapter.getRemoteDevice(macAddress);
            tvStatusConexao.setText("Status: Conectando...");
            tvStatusConexao.setTextColor(Color.YELLOW);

            // Conectar em uma thread separada para não travar a UI
            new Thread(() -> {
                try {
                    bluetoothSocket = device.createInsecureRfcommSocketToServiceRecord(MY_UUID);
                    bluetoothSocket.connect();
                    isConnected = true;

                    runOnUiThread(() -> {
                        btnConectar.setText("Desconectar");
                        tvStatusConexao.setText("Status: Conectado a " + device.getName());
                        tvStatusConexao.setTextColor(Color.GREEN);
                        btnIniciarCiclo.setEnabled(true);
                        btnAbrirMonitor.setEnabled(true);
                        Toast.makeText(this, "Conectado com sucesso!", Toast.LENGTH_SHORT).show();
                    });

                    connectedThread = new ConnectedThread(bluetoothSocket);
                    connectedThread.start();
                } catch (IOException e) {
                    isConnected = false;
                    runOnUiThread(() -> {
                        tvStatusConexao.setText("Status: Falha na conexão");
                        tvStatusConexao.setTextColor(Color.RED);
                        Toast.makeText(this, "Falha ao conectar: " + e.getMessage(), Toast.LENGTH_LONG).show();
                    });
                }
            }).start();
        }
    }

    // ... (outras funções de UI e verificação de permissão) ...

    private class ConnectedThread extends Thread {
        private final InputStream mmInStream;
        private final OutputStream mmOutStream;

        public ConnectedThread(BluetoothSocket socket) {
            InputStream tmpIn = null;
            OutputStream tmpOut = null;
            try {
                tmpIn = socket.getInputStream();
                tmpOut = socket.getOutputStream();
            } catch (IOException e) { Log.e(TAG, "Erro ao criar streams", e); }
            mmInStream = tmpIn;
            mmOutStream = tmpOut;
        }

        public void run() {
            BufferedReader reader = new BufferedReader(new InputStreamReader(mmInStream));
            while (isConnected) {
                try {
                    String line = reader.readLine();
                    if (line != null && MonitorActivity.handler != null) {
                        // Envia a linha completa para o handler da MonitorActivity
                        MonitorActivity.handler.obtainMessage(2, line).sendToTarget();
                    }
                } catch (IOException e) {
                    Log.d(TAG, "Input stream desconectado", e);
                    runOnUiThread(() -> desconectar());
                    break;
                }
            }
        }

        public void write(String input) {
            try {
                mmOutStream.write(input.getBytes());
            } catch (IOException e) { Log.e(TAG, "Erro ao enviar dados", e); }
        }
    }

    private void vincularComponentesUI() {
        btnConectar = findViewById(R.id.btnConectar);
        tvStatusConexao = findViewById(R.id.tvStatusConexao);
        etTemperatura = findViewById(R.id.etTemperatura);
        etTempoCiclo = findViewById(R.id.etTempoCiclo);
        etVolumeRefil = findViewById(R.id.etVolumeRefil);
        etVolumeExtracao = findViewById(R.id.etVolumeExtracao);
        btnIniciarCiclo = findViewById(R.id.btnIniciarCiclo);
        btnAbrirMonitor = findViewById(R.id.btnAbrirMonitor);
    }

    private void verificarPermissoesBluetooth() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            if (ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.BLUETOOTH_CONNECT, Manifest.permission.BLUETOOTH_SCAN}, 1);
            }
        }
    }
}
