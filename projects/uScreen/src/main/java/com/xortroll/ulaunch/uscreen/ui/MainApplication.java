package com.xortroll.ulaunch.uscreen.ui;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Optional;
import javafx.scene.control.ButtonBar.ButtonData;
import javafx.scene.control.ButtonType;
import javafx.scene.control.Dialog;
import javafx.application.Application;
import javafx.application.Platform;
import javafx.fxml.FXMLLoader;
import javafx.scene.Scene;
import javafx.scene.image.Image;
import javafx.scene.layout.Pane;
import javafx.stage.Stage;
import com.xortroll.ulaunch.uscreen.usb.UsbInterface;
import com.xortroll.ulaunch.uscreen.usb.UsbMode;

public class MainApplication extends Application {
    private MainController controller;
    private Scene scene;

    private UsbInterface usb_intf = null;

    private Object dialog_wait_lock = new Object();
    private boolean is_dialog_finished = false;

    private void die() {
        if(this.usb_intf != null) {
            this.usb_intf.finalize();
        }

        Platform.exit();
        System.exit(0);
    }

    // Note: code gracefully grabbed from Quark

    private void showOkDialogFromTask(String subtitle, String message, boolean terminate) {
        is_dialog_finished = false;

        Platform.runLater(() -> {
            Dialog<ButtonType> dialog = new Dialog<ButtonType>();
            dialog.setTitle("uScreen - " + subtitle);
            dialog.setContentText(message);

            ButtonType type = new ButtonType("Ok", ButtonData.OK_DONE);
            dialog.getDialogPane().getButtonTypes().add(type);

            dialog.showAndWait();

            if(terminate) {
                die();
            }
            synchronized(dialog_wait_lock) {
                is_dialog_finished = true;
            }
        });

        while(true) {
            synchronized(dialog_wait_lock) {
                if(is_dialog_finished) {
                    break;
                }
            }
        }
    }

    private UsbInterface showUsbFailReconnectDialogFromTask(String message, boolean initial_start_silent) {
        boolean skip_dialog = initial_start_silent;
        String show_message = message;
        while(true) {
            if(!skip_dialog) {
                is_dialog_finished = false;

                String dialog_msg = show_message;
                Platform.runLater(() -> {
                    Dialog<ButtonType> dialog = new Dialog<ButtonType>();
                    dialog.setTitle("uScreen - USB connection");
                    dialog.setContentText(dialog_msg);

                    dialog.getDialogPane().getButtonTypes().add(new ButtonType("Reconnect", ButtonData.YES));
                    dialog.getDialogPane().getButtonTypes().add(new ButtonType("Exit", ButtonData.NO));

                    dialog.showAndWait().ifPresent(ret_type -> {
                        if(ret_type.getButtonData() == ButtonData.NO) {
                            die();
                        }
                    });

                    synchronized(dialog_wait_lock) {
                        is_dialog_finished = true;
                    }
                });

                while(true) {
                    synchronized(dialog_wait_lock) {
                        if(is_dialog_finished) {
                            break;
                        }
                    }
                }
            }

            skip_dialog = false;

            // Try reconnecting, otherwise continue asking the user
            Optional<UsbInterface> maybe_intf = UsbInterface.createInterface(0);
            if(maybe_intf.isPresent()) {
                return maybe_intf.get();
            }
            else {
                show_message = "Unable to connect via USB";
            }
        }
    }

    @Override
    public void start(Stage primaryStage) throws Exception {
        ClassLoader this_loader = getClass().getClassLoader();
        FXMLLoader loader = new FXMLLoader(this_loader.getResource("Main.fxml"));
        Pane base = loader.load();

        double width = base.getPrefWidth();
        double height = base.getPrefHeight();

        this.scene = new Scene(base, width, height);
        this.scene.getStylesheets().add(this_loader.getResource("Main.css").toExternalForm());

        this.controller = loader.getController();
        
        Stage stage = primaryStage;
        stage.getIcons().add(new Image(this_loader.getResource("Icon.png").toExternalForm()));
        stage.setTitle("uScreen");
        stage.setScene(this.scene);
        stage.setMinWidth(width);
        stage.setMinHeight(height);

        controller.prepare(stage);
        stage.show();

        Thread usb_thread = new Thread() {
            public void run() {
                UsbInterface usb_intf = showUsbFailReconnectDialogFromTask("", true);

                ByteBuffer data = ByteBuffer.allocateDirect(8 + 1280 * 720 * 4);
                data.order(ByteOrder.LITTLE_ENDIAN);

                while(true) {
                    data.rewind();
                    if(!usb_intf.readByteBuffer(data, data.limit())) {
                        System.out.println("Unable to read data");
                        continue;
                    }

                    int mode = data.getInt();
                    if(mode == UsbMode.RAW_RGBA) {
                        controller.updateScreenRgba(data);
                    }
                    else if(mode == UsbMode.JPEG) {
                        int jpeg_size = data.getInt();
                        showOkDialogFromTask("Invalid mode", "JPEG mode is currently not supported (size: " + jpeg_size + ")", true);
                    }
                    else {
                        System.out.println("Invalid mode: " + mode);
                        continue;
                    }
                }
            }
        };
        usb_thread.setDaemon(true);
        usb_thread.start();
    }
    
    public static void run(String[] args) {
        Application.launch(args);
    }
}