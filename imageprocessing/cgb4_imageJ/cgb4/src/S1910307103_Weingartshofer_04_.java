/*
Title:      CGB Exercise 04
Author:     Florian Weingartshofer
Date:       04.06.2021
Time spent: 7h
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

        Optional<PathFindingMode> opMode = showModeDialog();
        if (opMode.isEmpty()) {
            return;
        }
        state.setMode(opMode.get());


        // **** Distance Map Without Safety Distance
        var distanceMapWithoutSafetyDistance = createDistanceMap(originalImage, Target.End);
        var scaledDistanceMapWithoutSafetyDistance = scaleDistanceMap(distanceMapWithoutSafetyDistance);
        var shortestPathWithoutSafetyDistance = calculateShortestPath(originalImage, distanceMapWithoutSafetyDistance);
        System.out.println("Path length without Safety Distance: " + shortestPathWithoutSafetyDistance.size());
        var shortestPathWithoutSafetyDistanceImage = drawPathInImage(originalImage, (List<Tuple<Integer, Integer>>) shortestPathWithoutSafetyDistance);

        // ***** Obstacle Map
        var obstacleDistanceMap = createDistanceMap(originalImage, Target.Obstacles);
        var obstacleDistanceMapScaled = scaleDistanceMap(obstacleDistanceMap);

        // ***** Distance Map With Safety Distance
        var distanceMapWithSafety = createDistanceMapWithSafetyDistance(originalImage, obstacleDistanceMap);
        var shortestPathWithSafety = calculateShortestPath(originalImage, distanceMapWithSafety);
        System.out.println("Path length with Safety Distance: " + shortestPathWithSafety.size());
        System.out.println("Difference: " + (shortestPathWithSafety.size() - shortestPathWithoutSafetyDistance.size()));

        var shortestPathWithSafetyImage = drawPathInImage(originalImage, shortestPathWithSafety);

        // ***** Draw Images
        GenericDialog showEverythingDialog = new GenericDialog("Show Everything?");
        showEverythingDialog.addCheckbox("Show Everything??", false);
        showEverythingDialog.showDialog();
        boolean showEverything = !showEverythingDialog.wasCanceled() && showEverythingDialog.getNextBoolean();

        if (showEverything) {
            normalizeDistanceMapAndDisplay(distanceMapWithoutSafetyDistance);
            ImageJUtility.showNewImage(scaledDistanceMapWithoutSafetyDistance, width, height, "Scaled Distance Map");
        }

        ImageJUtility.showNewImage(shortestPathWithoutSafetyDistanceImage, width, height, "Image Without Safety Distance");
        WindowManager.setTempCurrentImage(WindowManager.getImage("Image Without Safety Distance"));
        IJ.run("mazeLUT");

        if (showEverything) {
            ImageJUtility.showNewImage(obstacleDistanceMapScaled, state.getWidth(), state.getHeight(), "Obstacle Distance Map Scaled");
        }

        ImageJUtility.showNewImage(shortestPathWithSafetyImage, width, height, "Image With Safety Distance");
        WindowManager.setTempCurrentImage(WindowManager.getImage("Image With Safety Distance"));
        IJ.run("mazeLUT");
    }

    /**
     * Draws the path into the image. Does not change originalImage
     *
     * @param originalImage the original image
     * @param path          the relevant pixels for the path
     * @return the image with path
     */
    private int[][] drawPathInImage(int[][] originalImage, List<Tuple<Integer, Integer>> path) {
        int[][] clonedOriginal = cloneImage(originalImage);
        for (var point : path) {
            clonedOriginal[point.x][point.y] = PATH;
        }
        return clonedOriginal;
    }

    /**
     * Calculates the shortest path from the start point to end point
     *
     * @param originalImage used to find start and end point
     * @param distanceMap   used to find the path between start and end point
     * @return the relevant pixels of the path
     */
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

        // The remaining distance to the end point
        double remainingDistance = Double.POSITIVE_INFINITY;
        while (remainingDistance > 0) {
            var neighbours = neighborsOfPixel(start, distanceMap);
            var minNeighbourDistance = Double.POSITIVE_INFINITY;
            var prevMinDistance = Double.POSITIVE_INFINITY;
            var bestNeighbour = Tuple.of(0, 0);

            // Find next neighbour with smallest distance
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

    /**
     * Scales the distance map so there are negative or infinity values
     *
     * @param distanceMap to be scaled
     * @return a new distanceMap with scaled values
     */
    private double[][] scaleDistanceMap(double[][] distanceMap) {
        var scaledDistanceMap = new double[state.getWidth()][state.getHeight()];
        var maxValue = getMaxValueWithoutInfinity(distanceMap);

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

    /**
     * Finds the greatest value in a distance map excluding Infinity
     *
     * @param distanceMap contains the greatest value
     * @return the greatest value
     */
    private double getMaxValueWithoutInfinity(double[][] distanceMap) {
        double maxValue = Double.NEGATIVE_INFINITY;
        for (int x = 0; x < state.getWidth(); x++) {
            for (int y = 0; y < state.getHeight(); y++) {
                if (distanceMap[x][y] > maxValue && distanceMap[x][y] != Double.POSITIVE_INFINITY) {
                    maxValue = distanceMap[x][y];
                }
            }
        }
        return maxValue;
    }

    /**
     * Normalize a distance map, which has not been scaled
     * Mostly used for debugging
     *
     * @param distanceMap unscaled distance map
     */
    private void normalizeDistanceMapAndDisplay(double[][] distanceMap) {
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

    /**
     * Utility to show errors to the user
     *
     * @param message
     */
    private void showErrorDialog(String message) {
        GenericDialog errorDialog = new GenericDialog("ERROR");
        errorDialog.addMessage(message);
        errorDialog.showDialog();
    }

    /**
     * Checks that an image only contains th predefined values
     *
     * @param originalImage maze to check
     * @return if the maze is valid
     */
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

    /**
     * Asks the user what path finding mode should be used
     *
     * @return a path finding mode if one was selected
     */
    private Optional<PathFindingMode> showModeDialog() {
        // Convert an enum to a string array
        String[] modes = Arrays.stream(PathFindingMode.class.getEnumConstants())
                .map(Enum::name)
                .toArray(String[]::new);
        GenericDialog gd = new GenericDialog("Choose a mode: ");
        gd.addChoice("Mode", modes, modes[0]);
        gd.showDialog();
        if (gd.wasCanceled()) {
            return Optional.empty();
        }
        String modeStr = gd.getNextChoice();
        return Optional.of(PathFindingMode.valueOf(modeStr));
    }

    /**
     * Creates the distance map of an image
     *
     * @param originalImage the original image
     * @param target        if the distance map should target the end or obstacles
     * @return a new distance map
     */
    private double[][] createDistanceMap(int[][] originalImage, Target target) {
        var distanceMap = new double[state.getWidth()][state.getHeight()];
        Queue<Tuple<Integer, Integer>> pixelsOfInterest = new LinkedList<>();

        // Set Inaccessible pixels and target pixels
        // everything else is infinity
        // add the relevant pixels to a queue
        for (int x = 0; x < state.getWidth(); x++) {
            for (int y = 0; y < state.getHeight(); y++) {
                var targetIntensity = target == Target.End ? END : OBSTACLE;
                if (originalImage[x][y] == targetIntensity) {
                    distanceMap[x][y] = 0;
                    pixelsOfInterest.add(Tuple.of(x, y));
                } else if (originalImage[x][y] == WALL
                        || (target == Target.End && originalImage[x][y] == OBSTACLE)) {
                    distanceMap[x][y] = INACCESSIBLE;
                } else {
                    distanceMap[x][y] = Double.POSITIVE_INFINITY;
                }
            }
        }


        // Iterate over all relevant pixels and calculate the values of their neighbours
        // If a neighbour is also relevant it will be added to the queue
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

    /**
     * Create a distance map but with an safety distance to obstacles
     *
     * @param originalImage the original image
     * @param obstacleMap   the distance map of the obstacles
     * @return a distance map with safety distance
     */
    private double[][] createDistanceMapWithSafetyDistance(int[][] originalImage, double[][] obstacleMap) {
        int[][] imageWithObstacles = cloneImage(originalImage);
        for (int x = 0; x < state.getWidth(); x++) {
            for (int y = 0; y < state.getHeight(); y++) {
                if (obstacleMap[x][y] <= SAFETY_DISTANCE) {
                    imageWithObstacles[x][y] = WALL;
                }
            }
        }

        return createDistanceMap(imageWithObstacles, Target.End);
    }

    /**
     * Calculate the distance to a neighbour
     * The value of the distance is determined by the path finding mode
     *
     * @param pixel     the start pixel
     * @param neighbour the neighbour of the pixel
     * @return the distance to a neighbour
     */
    private double distanceToNeighbour(Tuple<Integer, Integer> pixel, Tuple<Integer, Integer> neighbour) {
        // vertical/horizontal is always one and for checkerboard diagonal is also one
        if (pixel.x.equals(neighbour.x) || pixel.y.equals(neighbour.y)
                || state.getMode() == PathFindingMode.Checkerboard) {
            return 1;
        }
        if (state.getMode() == PathFindingMode.Euclid) {
            return Math.sqrt(2);
        }
        return Double.POSITIVE_INFINITY;
    }

    /**
     * Find all relevant neighbours of a pixel. Not relevant pixels are for example pixels with their value below zero
     *
     * @param coordinate  the coordinate of the pixel
     * @param distanceMap the distance map to find the neigbours
     * @return a list of relevant neighbours
     */
    private List<Tuple<Integer, Integer>> neighborsOfPixel(Tuple<Integer, Integer> coordinate, double[][] distanceMap) {
        List<Tuple<Integer, Integer>> neighbours = new ArrayList<>();
        for (int x = -1; x <= 1; x++) {
            for (int y = -1; y <= 1; y++) {
                final var neighbour = Tuple.of(coordinate.x + x, coordinate.y + y);
                if (!isValidCoordinate(neighbour) // Outside of the image
                        || (state.getMode() == PathFindingMode.Manhattan && (x != 0 && y != 0)) // For Manhattan diagonal neighbours need to be excluded
                        || (distanceMap[neighbour.x][neighbour.y] < 0)) { // could be a wall or obstacle
                    continue;
                }
                neighbours.add(neighbour);
            }
        }
        return neighbours;
    }

    /**
     * Check if a tuple is a valid coordinate in the image
     *
     * @param coordinate coordinate which is checked
     * @return if the coordinate is valid
     */
    private boolean isValidCoordinate(Tuple<Integer, Integer> coordinate) {
        return coordinate.x >= 0
                && coordinate.y >= 0
                && coordinate.x < state.getWidth()
                && coordinate.y < state.getHeight();
    }

    /**
     * Deep clones an image
     *
     * @param arr the image to be cloned
     * @return a new image
     */
    private int[][] cloneImage(int[][] arr) {
        var newImage = new int[state.getWidth()][state.getHeight()];
        for (int i = 0; i < state.getWidth(); i++) {
            System.arraycopy(arr[i], 0, newImage[i], 0, arr[i].length);
        }
        return newImage;
    }

    /**
     * Holds the state of the Plugin:
     * Image Width, Height and the path finding mode
     */
    static class State {
        private final int width;
        private final int height;
        private PathFindingMode pathFindingMode;

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

        public PathFindingMode getMode() {
            return pathFindingMode;
        }

        public void setMode(PathFindingMode pathFindingMode) {
            this.pathFindingMode = pathFindingMode;
        }
    }

    /**
     * Utility class to hold two generic values
     * Used as 2D Coordinate
     *
     * @param <X>
     * @param <Y>
     */
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

    /**
     * The path finding mode
     */
    enum PathFindingMode {
        Checkerboard, Euclid, Manhattan
    }

    /**
     * Defines which object the distanceMap Algorithm has to target
     */
    enum Target {
        End, Obstacles
    }
}
