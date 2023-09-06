package com.xortroll.ulaunch.uscreen.ui;

import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;
import javax.imageio.ImageIO;
import javafx.stage.Stage;
import javafx.stage.FileChooser;
import javafx.scene.control.Button;
import javafx.scene.image.ImageView;
import javafx.scene.image.PixelFormat;
import javafx.scene.image.PixelWriter;
import javafx.scene.image.WritableImage;
import java.awt.image.BufferedImage;
import javafx.embed.swing.SwingFXUtils;
import javafx.fxml.FXML;

public class MainController {
    private Stage stage;
    @FXML private ImageView ScreenImage;
    @FXML private Button ScreenshotButton;

    public void prepare(Stage stage) {
        this.stage = stage;

        WritableImage img = new WritableImage(1280, 720);
        this.ScreenImage.setImage(img);

        this.ScreenshotButton.setOnAction(event -> {
            BufferedImage buf_img = SwingFXUtils.fromFXImage(this.ScreenImage.getImage(), null);

            FileChooser file_ch = new FileChooser();
            file_ch.setTitle("Save screenshot");
            File selected_f = file_ch.showSaveDialog(this.stage);
            if(selected_f != null) {
                try {
                    ImageIO.write(buf_img, "png", selected_f);
                }
                catch(IOException e) {
                    System.out.println("Exception saving screenshot: " + e.toString());
                }
            }
        });
    }

    public void updateScreenRgba(ByteBuffer rgba_buf) {
        PixelWriter writer = ((WritableImage)this.ScreenImage.getImage()).getPixelWriter();
        
        // TODONEW: gross way of converting from RGBA to BGRA, any better way to do it here in uScreen?
        for(int i = 0; i < 1280*720*4; i += 4) {
            byte tmp_r = rgba_buf.get(i + 0);
            byte tmp_b = rgba_buf.get(i + 2);
            rgba_buf.put(i + 0, tmp_b);
            rgba_buf.put(i + 2, tmp_r);
        }

        writer.setPixels(0, 0, 1280, 720, PixelFormat.getByteBgraInstance(), rgba_buf, 1280 * 4);
    }
}