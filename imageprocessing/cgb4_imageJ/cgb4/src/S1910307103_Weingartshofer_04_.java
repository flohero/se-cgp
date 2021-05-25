/*
Title:      CGB Exercise 04
Author:     Florian Weingartshofer
Date:       25.05.2021
Time spent: TODO
Notes:      Requires mazeLUT.lut and Java 11
*/

import ij.ImagePlus;
import ij.gui.GenericDialog;
import ij.plugin.filter.PlugInFilter;
import ij.process.ImageProcessor;

import javax.swing.text.html.Option;
import java.util.Arrays;
import java.util.LinkedList;
import java.util.Optional;
import java.util.Queue;

public class S1910307103_Weingartshofer_04_ implements PlugInFilter {

    /* Intensities */
    private static final int BACKGROUND = 255;
    private static final int WALL = 0;
    private static final int START = 60;
    private static final int END = 120;
    private static final int OBSTACLE = 180;

    // Intensity of the path, will be colored with a Lookup Table
    private static final int PATH = 50;

    private static final int INACCESSIBLE = -1;

    private static final int SAFETY_DISTANCE = 4;

    private ImageInfo imageInfo;


    @Override
    public int setup(String s, ImagePlus imagePlus) {
        return DOES_8G + DOES_STACKS + SUPPORTS_MASKING;
    }

    @Override
    public void run(ImageProcessor imageProcessor) {
        byte[] pixels = (byte[]) imageProcessor.getPixels();

        int width = imageProcessor.getWidth();
        int height = imageProcessor.getHeight();

        imageInfo = new ImageInfo(width, height);

        int[][] originalImage = ImageJUtility.convertFrom1DByteArr(pixels, width, height);

        if (!isValidMaze(originalImage)) {
            showErrorDialog("Image contains invalid pixels!");
            return;
        }

        Optional<Mode> opMode = showModeDialog();
        if (opMode.isEmpty()) {
            return;
        }

        var distanceMapWithoutSafetyDistance = createDistanceMap(originalImage, Target.End);


    }


    private void showErrorDialog(String message) {
        GenericDialog errorDialog = new GenericDialog("ERROR");
        errorDialog.addMessage(message);
        errorDialog.showDialog();
    }

    private boolean isValidMaze(int[][] originalImage) {
        return Arrays.stream(originalImage)
                .flatMapToInt(Arrays::stream)
                .anyMatch(pixel -> pixel != BACKGROUND
                        && pixel != WALL
                        && pixel != OBSTACLE
                        && pixel != START
                        && pixel != END
                );
    }

    private Optional<Mode> showModeDialog() {
        String[] modes = Arrays.stream(Mode.class.getEnumConstants())
                .map(Enum::name)
                .toArray(String[]::new);
        GenericDialog gd = new GenericDialog("Choose a mode: ");
        gd.addChoice("Mode", modes, modes[0]);
        gd.showDialog();
        if (gd.wasCanceled()) {
            return Optional.empty();
        }
        String modeStr = gd.getNextChoice();
        return Optional.of(Mode.valueOf(modeStr));
    }

    private double[][] createDistanceMap(int[][] originalImage, Target target) {
        var distanceMap = new double[imageInfo.getWidth()][imageInfo.getHeight()];
        Queue<Tuple<Integer, Integer>> pixelsOfInterest = new LinkedList<>();
        for (int x = 0; x < imageInfo.getWidth(); x++) {
            for (int y = 0; y < imageInfo.getHeight(); y++) {
                var endIsTarget = target == Target.End;
                final int pixel = originalImage[x][y];
                pixelsOfInterest.add(Tuple.of(x, y));
                if (endIsTarget && pixel == END || !endIsTarget && pixel == OBSTACLE) {
                    distanceMap[x][y] = pixel;
                } else if (pixel == WALL || endIsTarget && pixel == OBSTACLE) {
                    distanceMap[x][y] = pixel;
                } else {
                    distanceMap[x][y] = Double.POSITIVE_INFINITY;
                }
            }
        }

        //TODO: update pixels of interest

        return distanceMap;
    }

    static class ImageInfo {
        private final int width;
        private final int height;

        ImageInfo(int width, int height) {
            this.width = width;
            this.height = height;
        }

        public int getWidth() {
            return width;
        }

        public int getHeight() {
            return height;
        }
    }

    static class Tuple<X, Y> {
        private final X x;
        private final Y y;

        public Tuple(X x, Y y) {
            this.x = x;
            this.y = y;
        }

        public static <X, Y> Tuple<X, Y> of(X x, Y y) {
            return new Tuple<>(x, y);
        }

        public X x() {
            return x;
        }

        public Y y() {
            return y;
        }
    }

    enum Mode {
        Checkerboard, Euclid, Manhattan
    }

    enum Target {
        End, Obstacles
    }
}
