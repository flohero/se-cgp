/*
Title:      CGB Exercise 04
Author:     Florian Weingartshofer
Date:       25.05.2021
Time spent: TODO
Notes:      Requires mazeLUT.lut and Java 11
*/

import ij.IJ;
import ij.ImagePlus;
import ij.WindowManager;
import ij.gui.GenericDialog;
import ij.plugin.filter.PlugInFilter;
import ij.process.ImageProcessor;

import java.util.*;

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
    public static final double MAX_SCALE_VALUE = 200.0;

    private State state;


    @Override
    public int setup(String s, ImagePlus imagePlus) {
        return DOES_8G + DOES_STACKS + SUPPORTS_MASKING;
    }

    @Override
    public void run(ImageProcessor imageProcessor) {
        byte[] pixels = (byte[]) imageProcessor.getPixels();

        int width = imageProcessor.getWidth();
        int height = imageProcessor.getHeight();

        state = new State(width, height);

        int[][] originalImage = ImageJUtility.convertFrom1DByteArr(pixels, width, height);

        if (!isValidMaze(originalImage)) {
            showErrorDialog("Image contains invalid pixels!");
            return;
        }

        Optional<Mode> opMode = showModeDialog();
        if (opMode.isEmpty()) {
            return;
        }
        state.setMode(opMode.get());

        // **** Distance Map Without Safety Distance
        var distanceMapWithoutSafetyDistance = createDistanceMap(originalImage, Target.End);
        displayDistanceMap(distanceMapWithoutSafetyDistance);

        double[][] scaledDistanceMapWithoutSafetyDistance = scaleDistanceMap(distanceMapWithoutSafetyDistance);
        ImageJUtility.showNewImage(scaledDistanceMapWithoutSafetyDistance, width, height, "Scaled Distance Map");

        var shortestPathWithoutSafetyDistance = calculateShortestPath(originalImage, distanceMapWithoutSafetyDistance);

        int[][] shortestPathWithoutSafetyDistanceImage = cloneImage(originalImage);
        for (var path : shortestPathWithoutSafetyDistance) {
            shortestPathWithoutSafetyDistanceImage[path.x][path.y] = PATH;
        }

        ImageJUtility.showNewImage(shortestPathWithoutSafetyDistanceImage, width, height, "Image Without Safety Distance");
        WindowManager.setTempCurrentImage(WindowManager.getImage("Image Without Safety Distance"));
        IJ.run("mazeLUT");

        // ***** Obstacle Map
        double[][] obstacleDistanceMap = createDistanceMap(originalImage, Target.Obstacles);
        double[][] obstacleDistanceMapScaled = scaleDistanceMap(obstacleDistanceMap);
        ImageJUtility.showNewImage(obstacleDistanceMapScaled, state.getWidth(), state.getHeight(), "Obstacle Distance Map Scaled");

        // ***** Distance Map With Safety Distance


    }

    private Vector<Tuple<Integer, Integer>> calculateShortestPath(int[][] originalImage, double[][] distanceMap) {
        Vector<Tuple<Integer, Integer>> shortestPath = new Vector<>();

        List<Tuple<Integer, Integer>> startCoordinates = new ArrayList<>();

        // Find all start pixels
        for (int x = 0; x < state.getWidth(); x++) {
            for (int y = 0; y < state.getHeight(); y++) {
                if (originalImage[x][y] == START) {
                    startCoordinates.add(Tuple.of(x, y));
                }
            }
        }

        var minDistance = Double.POSITIVE_INFINITY;
        var start = Tuple.of(0, 0);
        // Find start pixel with min distance
        for (var coordinate : startCoordinates) {
            var distance = distanceMap[coordinate.x][coordinate.y];
            if (distance < minDistance) {
                minDistance = distance;
                start = Tuple.of(coordinate.x, coordinate.y);
            }
        }

        shortestPath.add(start);
        double remainingDistance = Double.POSITIVE_INFINITY;
        while (remainingDistance > 0) {
            var neighbours = neighborsOfPixel(start, distanceMap);
            var minNeighbourDistance = Double.POSITIVE_INFINITY;
            var prevMinDistance = Double.POSITIVE_INFINITY;
            var bestNeighbour = Tuple.of(0, 0);

            for (var neighbour : neighbours) {
                double currentMoveCost = distanceToNeighbour(start, neighbour);
                final double neighbourDistance = distanceMap[neighbour.x][neighbour.y];
                if (neighbourDistance + currentMoveCost < minNeighbourDistance + prevMinDistance) {
                    prevMinDistance = currentMoveCost;
                    minNeighbourDistance = neighbourDistance;
                    bestNeighbour = Tuple.of(neighbour.x, neighbour.y);
                }
            }

            remainingDistance = minNeighbourDistance;
            shortestPath.add(bestNeighbour);
            start = bestNeighbour;
        }

        return shortestPath;
    }

    private double[][] scaleDistanceMap(double[][] distanceMap) {
        var scaledDistanceMap = new double[state.getWidth()][state.getHeight()];

        double maxValue = Double.NEGATIVE_INFINITY;
        maxValue = getMaxValueWithoutInfinity(distanceMap, maxValue);

        for (int x = 0; x < state.getWidth(); x++) {
            for (int y = 0; y < state.getHeight(); y++) {
                if (distanceMap[x][y] <= 0) {
                    scaledDistanceMap[x][y] = 255.0;
                } else if (distanceMap[x][y] == Double.POSITIVE_INFINITY) {
                    scaledDistanceMap[x][y] = 0.0;
                } else {
                    scaledDistanceMap[x][y] = (distanceMap[x][y] / maxValue) * MAX_SCALE_VALUE;
                }
            }
        }

        return scaledDistanceMap;
    }

    private double getMaxValueWithoutInfinity(double[][] distanceMap, double maxValue) {
        for (int x = 0; x < state.getWidth(); x++) {
            for (int y = 0; y < state.getHeight(); y++) {
                if (distanceMap[x][y] > maxValue && distanceMap[x][y] != Double.POSITIVE_INFINITY) {
                    maxValue = distanceMap[x][y];
                }
            }
        }
        return maxValue;
    }

    private void displayDistanceMap(double[][] distanceMap) {
        var displayMap = new double[state.getWidth()][state.getHeight()];
        for (int x = 0; x < state.getWidth(); x++) {
            for (int y = 0; y < state.getHeight(); y++) {
                if (distanceMap[x][y] > 255) {
                    displayMap[x][y] = 255;
                } else if (distanceMap[x][y] < 0) {
                    displayMap[x][y] = 0;
                } else {
                    displayMap[x][y] = distanceMap[x][y];
                }
            }
        }

        ImageJUtility.showNewImage(displayMap, state.getWidth(), state.getHeight(), "Distance Map Unscaled");
    }

    private void showErrorDialog(String message) {
        GenericDialog errorDialog = new GenericDialog("ERROR");
        errorDialog.addMessage(message);
        errorDialog.showDialog();
    }

    private boolean isValidMaze(int[][] originalImage) {
        for (int x = 0; x < state.getWidth(); x++) {
            for (int y = 0; y < state.getHeight(); y++) {
                var pixel = originalImage[x][y];
                if (!(pixel == BACKGROUND || pixel == WALL ||
                        pixel == START || pixel == END || pixel == OBSTACLE)) {
                    return false;
                }
            }
        }
        return true;
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
        var distanceMap = new double[state.getWidth()][state.getHeight()];
        Queue<Tuple<Integer, Integer>> pixelsOfInterest = new ArrayDeque<>();

        for (int x = 0; x < state.getWidth(); x++) {
            for (int y = 0; y < state.getHeight(); y++) {

                // For the End Target Map
                if (target == Target.End) {
                    if (originalImage[x][y] == END) {
                        distanceMap[x][y] = 0;
                        pixelsOfInterest.add(Tuple.of(x, y));
                    } else if (originalImage[x][y] == WALL || originalImage[x][y] == OBSTACLE) {
                        distanceMap[x][y] = INACCESSIBLE;
                    } else {
                        distanceMap[x][y] = Double.POSITIVE_INFINITY;
                    }
                } else { // For the Obstacle Map
                    if (originalImage[x][y] == OBSTACLE) {
                        distanceMap[x][y] = 0;
                        pixelsOfInterest.add(Tuple.of(x, y));
                    } else if (originalImage[x][y] == WALL) {
                        distanceMap[x][y] = INACCESSIBLE;
                    } else {
                        distanceMap[x][y] = Double.POSITIVE_INFINITY;
                    }
                }
            }
        }

        while (!pixelsOfInterest.isEmpty()) {
            Tuple<Integer, Integer> tuple = pixelsOfInterest.poll();
            var neighbours = neighborsOfPixel(tuple, distanceMap);

            for (var neighbour : neighbours) {
                double distanceToNeighbour = distanceToNeighbour(tuple, neighbour);
                final double neighbourValue = distanceMap[neighbour.x][neighbour.y];

                if (distanceMap[tuple.x][tuple.y] > neighbourValue + distanceToNeighbour) {
                    distanceMap[tuple.x][tuple.y] = neighbourValue + distanceToNeighbour;
                }
            }

            for (var neighbour : neighbours) {
                double distanceToNeighbor = distanceToNeighbour(tuple, neighbour);

                final double ownValue = distanceMap[tuple.x][tuple.y];
                if (distanceMap[neighbour.x][neighbour.y] > ownValue + distanceToNeighbor) {

                    distanceMap[neighbour.x][neighbour.y] = ownValue + distanceToNeighbor;
                    if (!pixelsOfInterest.contains(neighbour)) {
                        pixelsOfInterest.add(neighbour);
                    }
                }
            }
        }
        return distanceMap;
    }

    private double distanceToNeighbour(Tuple<Integer, Integer> tuple, Tuple<Integer, Integer> neighbour) {
        // vertical/horizontal is always one and for checkerboard diagonal is also one
        if (tuple.x.equals(neighbour.x) || tuple.y.equals(neighbour.y) || state.getMode() == Mode.Checkerboard) {
            return 1;
        }
        if (state.getMode() == Mode.Euclid) {
            return Math.sqrt(2);
        }
        return Double.POSITIVE_INFINITY;
    }

    private List<Tuple<Integer, Integer>> neighborsOfPixel(Tuple<Integer, Integer> tuple, double[][] distanceMap) {
        List<Tuple<Integer, Integer>> neighbours = new ArrayList<>();
        for (int x = -1; x <= 1; x++) {
            for (int y = -1; y <= 1; y++) {
                final var neighbour = Tuple.of(tuple.x + x, tuple.y + y);
                if (!isValidCoordinate(neighbour) // Outside of the image
                        || (state.getMode() == Mode.Manhattan && (x != 0 && y != 0)) // For Manhattan diagnoal neighbours need to be excluded
                        || (distanceMap[neighbour.x][neighbour.y] < 0.0)) {
                    continue;
                }
                neighbours.add(neighbour);
            }
        }
        return neighbours;
    }

    private boolean isValidCoordinate(Tuple<Integer, Integer> coordinate) {
        return coordinate.x >= 0
                && coordinate.y >= 0
                && coordinate.x < state.getWidth()
                && coordinate.y < state.getHeight();
    }

    private int[][] cloneImage(int[][] arr) {
        var newImage = new int[state.getWidth()][state.getHeight()];
        for (int i = 0; i < state.getWidth(); i++) {
            System.arraycopy(arr[i], 0, newImage[i], 0, arr[i].length);
        }
        return newImage;
    }

    static class State {
        private final int width;
        private final int height;
        private Mode mode;

        State(int width, int height) {
            this.width = width;
            this.height = height;
        }

        public int getWidth() {
            return width;
        }

        public int getHeight() {
            return height;
        }

        public Mode getMode() {
            return mode;
        }

        public void setMode(Mode mode) {
            this.mode = mode;
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

        @Override
        public boolean equals(Object obj) {
            return obj instanceof Tuple
                    && Objects.equals(((Tuple) obj).x, this.x)
                    && Objects.equals(((Tuple) obj).y, this.x);
        }
    }

    enum Mode {
        Checkerboard, Euclid, Manhattan
    }

    enum Target {
        End, Obstacles
    }
}
