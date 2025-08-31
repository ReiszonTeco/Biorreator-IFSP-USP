package com.example.bt_esp32;

import android.Manifest;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.core.app.ActivityCompat;

import java.util.Set;

public class DeviceListActivity extends Activity {

    public static String EXTRA_DEVICE_ADDRESS = "device_address";
    private BluetoothAdapter bluetoothAdapter;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        ListView listView = new ListView(this);
        setContentView(listView);

        ArrayAdapter<String> adapter = new ArrayAdapter<>(this, android.R.layout.simple_list_item_1);
        listView.setAdapter(adapter);

        bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();

        // Checa permissão antes de listar os dispositivos
        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
            // Em um app real, você solicitaria a permissão aqui.
            // Como a MainActivity já faz isso, vamos apenas avisar.
            Toast.makeText(this, "Permissão de Bluetooth não concedida", Toast.LENGTH_SHORT).show();
            finish();
            return;
        }

        Set<BluetoothDevice> pairedDevices = bluetoothAdapter.getBondedDevices();

        if (pairedDevices.size() > 0) {
            for (BluetoothDevice device : pairedDevices) {
                adapter.add(device.getName() + "\n" + device.getAddress());
            }
        } else {
            adapter.add("Nenhum dispositivo pareado encontrado");
        }

        listView.setOnItemClickListener((parent, view, position, id) -> {
            String info = ((TextView) view).getText().toString();
            if (info.equals("Nenhum dispositivo pareado encontrado")) return;

            String address = info.substring(info.length() - 17);

            Intent returnIntent = new Intent();
            returnIntent.putExtra(EXTRA_DEVICE_ADDRESS, address);
            setResult(Activity.RESULT_OK, returnIntent);
            finish();
        });
    }
}
