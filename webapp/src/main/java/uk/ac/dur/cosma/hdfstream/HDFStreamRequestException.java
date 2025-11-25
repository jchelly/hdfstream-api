package uk.ac.dur.cosma.hdfstream;

public class HDFStreamRequestException extends Exception {
    private final int statusCode;
    private final String errorMessage;

    public HDFStreamRequestException(int statusCode, String errorMessage) {
        super(errorMessage);
        this.statusCode = statusCode;
        this.errorMessage = errorMessage;
    }
    public int getStatusCode() { return statusCode; }
    public String getErrorMessage() { return errorMessage; }
}
