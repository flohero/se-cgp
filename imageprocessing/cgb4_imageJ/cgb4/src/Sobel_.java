import ij.IJ;
import ij.ImagePlus;
import ij.gui.GenericDialog;
import ij.plugin.filter.PlugInFilter;
import ij.process.ImageProcessor;

/**
 * Applies a sobel edge detection (high-pass filter) onto a given image. (Lab 6)
 */
public class Sobel_ implements PlugInFilter {

    public int setup(String arg, ImagePlus imp) {
        if (arg.equals("about")) {
            showAbout();
            return DONE;
        }
        return DOES_8G + DOES_STACKS + SUPPORTS_MASKING;
    } //setup


    public void run(ImageProcessor ip) {
        byte[] pixels = (byte[]) ip.getPixels();
        int width = ip.getWidth();
        int height = ip.getHeight();
        int[][] inDataArrInt = ImageJUtility.convertFrom1DByteArr(pixels, width, height);
        double[][] inDataArrDbl =
                ImageJUtility.convertToDoubleArr2D(inDataArrInt, width, height);

        GenericDialog gd = new GenericDialog("User Input");
        gd.addCheckbox("User vertical", true);
        gd.showDialog();

        if(gd.wasCanceled()) {
            return;
        }

        double[][] sobelV = new double[][]{
                {1, 2, 1},
                {0, 0, 0},
                {-1, -2, -1}
        };

        double[][] sobelH = new double[][]{
                {1, 0, -1},
                {2, 0, -2},
                {1, 0, -1}
        };

        double[][] mask = sobelV;
        if(!gd.getNextBoolean()) {
            mask = sobelH;
        }

        double[][] resultImg = ConvolutionFilter.convolveDouble(inDataArrDbl, width, height, mask, 1);

        double maxVal = Double.MIN_VALUE;
        for (int x = 0; x < width; x++) {
            for (int y = 0; y < height; y++) {
                final double valAbs = Math.abs(resultImg[x][y]);
                if (valAbs > maxVal) {
                    maxVal = valAbs;
                }
            }
        }

        double scaleFactor = 255.0 / maxVal;
        for (int x = 0; x < width; x++) {
            for (int y = 0; y < height; y++) {
                resultImg[x][y] = Math.abs(resultImg[x][y]) * scaleFactor;
            }
        }


        ImageJUtility.showNewImage(resultImg, width, height, "sobel ");

    } //run

    void showAbout() {
        IJ.showMessage("About Template_...",
                "this is a PluginFilter template\n");
    } //showAbout

} //class Mean_

