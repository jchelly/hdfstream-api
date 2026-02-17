import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.AfterAll;
import static org.junit.jupiter.api.Assertions.*;

import org.apache.commons.compress.archivers.tar.*;
import java.util.*;

import org.apache.http.auth.AuthScope;
import org.apache.http.auth.UsernamePasswordCredentials;
import org.apache.http.client.CredentialsProvider;
import org.apache.http.impl.client.BasicCredentialsProvider;
import org.apache.http.impl.client.HttpClientBuilder;
import org.apache.http.client.methods.CloseableHttpResponse;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.client.methods.HttpUriRequest;
import org.apache.http.impl.client.CloseableHttpClient;
import org.apache.http.entity.ByteArrayEntity;
import org.apache.http.impl.client.HttpClients;
import org.apache.http.client.utils.URIBuilder;
import org.apache.http.util.EntityUtils;
import org.apache.http.HttpEntity;
import org.apache.http.Header;
import java.net.URI;

import org.msgpack.core.MessagePack;
import org.msgpack.core.MessageUnpacker;
import org.msgpack.core.MessageBufferPacker;

import uk.ac.dur.cosma.hdfstream.SliceInfo;
import uk.ac.dur.cosma.hdfstream.InvalidSliceException;


public class UnitTestClient {

    private CloseableHttpClient client;
    private String baseURL;

    public UnitTestClient(String baseURL) throws Exception {
        this.baseURL = baseURL;
        this.client =  HttpClients.createDefault();
    }

    public UnitTestClient(String baseURL, String username, String password) throws Exception {
        this.baseURL = baseURL;
        CredentialsProvider provider = new BasicCredentialsProvider();
        provider.setCredentials(AuthScope.ANY, new UsernamePasswordCredentials(username, password));
        client = HttpClientBuilder.create().setDefaultCredentialsProvider(provider).build();
    }

    protected HttpGet makeGetRequest(String path, String object, String slice, String max_depth, String data_size_limit) throws Exception {
        // Build a http GET request with the specified parameters
        URIBuilder builder = new URIBuilder(baseURL+"/msgpack"+path);
        if(object != null)builder.addParameter("object", object);
        if(slice != null)builder.addParameter("slice", slice);
        if(max_depth != null)builder.addParameter("max_depth", max_depth);
        if(data_size_limit != null)builder.addParameter("data_size_limit", data_size_limit);
        URI uri = builder.build();
        return new HttpGet(uri);
    }

    protected HttpPost makePostRequest(String path, String object, String slice, String max_depth, String data_size_limit) throws Exception {
        // Build a http POST request with the specified parameters.
        // First, count how many parameters we have.
        int nr_params = 0;
        assertNotNull(path);
        if(object != null) nr_params += 1;
        if(slice != null) nr_params += 1;
        if(max_depth != null) nr_params += 1;
        if(data_size_limit != null) nr_params += 1;
        // Serialize the parameters
        MessageBufferPacker packer = MessagePack.newDefaultBufferPacker();
        packer.packMapHeader(nr_params);
        if(object != null) {
            packer.packString("object");
            packer.packString(object);
        }
        if(slice != null) {
            packer.packString("slice");
            // Translate the slice string to nested msgpack arrays
            SliceInfo si = new SliceInfo(slice);
            si.pack(packer);
        }
        if(max_depth != null) {
            packer.packString("max_depth");
            packer.packInt(Integer.valueOf(max_depth));
        }
        if(data_size_limit != null) {
            packer.packString("data_size_limit");
            packer.packLong(Long.valueOf(data_size_limit));
        }
        packer.close();
        byte[] msgpackBytes = packer.toByteArray();
        // Get the request URI
        URIBuilder builder = new URIBuilder(baseURL+"/msgpack"+path);
        URI uri = builder.build();
        // Create the request
        HttpPost post = new HttpPost(uri);
        post.setEntity(new ByteArrayEntity(msgpackBytes));
        post.setHeader("Content-Type", "application/x-msgpack");
        return post;
    }

    protected HttpUriRequest makeRequest(String path, String object, String slice, String max_depth, String data_size_limit, boolean post) throws Exception {
        if(post) {
            return makePostRequest(path, object, slice, max_depth, data_size_limit);
        } else {
            return makeGetRequest(path, object, slice, max_depth, data_size_limit);
        }
    }

    public DirectoryEntry requestDirectoryEntry(String path, String max_depth, int expected_status, boolean post) throws Exception {

        // Construct URI with the path and optional max_depth parameter
        HttpUriRequest request = makeRequest(path, null, null, max_depth, null, post);

        // Send the request and decode the response
        DirectoryEntry entry = null;
        try (CloseableHttpResponse response = client.execute(request)) {
            assertEquals(expected_status, response.getStatusLine().getStatusCode());
            if(expected_status==200) {
                HttpEntity entity = response.getEntity();
                byte[] data = EntityUtils.toByteArray(entity);
                MessageUnpacker unpacker = MessagePack.newDefaultUnpacker(data);
                entry = new DirectoryEntry(unpacker);
            }
        }
        return entry;
    }

    public DirectoryEntry requestDirectoryEntry(String path, int expected_status, boolean post) throws Exception {
        return requestDirectoryEntry(path, null, expected_status, post);
    }

    public NDArray requestSlices(String path, String object, String slice, int expected_status, boolean post) throws Exception {

        // Construct URI with the path, HDF5 object name, and slice specifier.
        // For GET requests the slice specifier is a string. For POSTs we
        // translate the slice specifier into msgpack arrays of integers.
        HttpUriRequest request;
        try {
            request = makeRequest(path, object, slice, null, null, post);
        } catch (InvalidSliceException e) {
            assertEquals(400, expected_status); // Should only happen if we expected a bad request error
            assertEquals(true, post);           // and this was a post request
            return null;
        }

        // Send the request and decode the response
        NDArray array = null;
        try (CloseableHttpResponse response = client.execute(request)) {
            assertEquals(expected_status, response.getStatusLine().getStatusCode());
            if(expected_status==200) {
                HttpEntity entity = response.getEntity();
                byte[] data = EntityUtils.toByteArray(entity);
                MessageUnpacker unpacker = MessagePack.newDefaultUnpacker(data);
                array = new NDArray(unpacker);
            }
        }
        return array;
    }

    public HDF5Object requestObject(String path, String object, String max_depth, String data_size_limit, int expected_status, boolean post) throws Exception {

        // Construct URI with the path and HDF5 object name
        HttpUriRequest request = makeRequest(path, object, null, max_depth, data_size_limit, post);

        // Send the request and decode the response
        HDF5Object h5obj = null;
        try (CloseableHttpResponse response = client.execute(request)) {
            assertEquals(expected_status, response.getStatusLine().getStatusCode());
            if(expected_status==200) {
                HttpEntity entity = response.getEntity();
                byte[] data = EntityUtils.toByteArray(entity);
                MessageUnpacker unpacker = MessagePack.newDefaultUnpacker(data);
                h5obj = new HDF5Object(unpacker, object);
            }
        }
        return h5obj;
    }

    public byte[] requestFile(String path, int expected_status) throws Exception {

	byte[] data = null;

        // Construct URI with the path
        URIBuilder builder = new URIBuilder(baseURL+"/download"+path);
        URI uri = builder.build();

        // Send the request and decode the response
        HttpGet request = new HttpGet(uri);
        try (CloseableHttpResponse response = client.execute(request)) {
            assertEquals(expected_status, response.getStatusLine().getStatusCode());
            if(expected_status==200) {
		// Get the request body as an array of bytes
                HttpEntity entity = response.getEntity();
                data = EntityUtils.toByteArray(entity);
		// For file downloads the content length should always be set
		Header contentLengthHeader = response.getFirstHeader("Content-Length");
		assertNotNull(contentLengthHeader);
		int length = Integer.parseInt(contentLengthHeader.getValue());
		assertEquals(data.length, length);
            }
        }
        return data;
    }

    // Request a tar file, unpack it and return a map of (path, byte array) pairs with the file contents
    public HashMap<String, byte[]> requestTarFile(String path, int expected_status) throws Exception {

	HashMap<String, byte[]> data = null;

        // Construct URI with the path to the tar file
        URIBuilder builder = new URIBuilder(baseURL+"/download"+path);
        URI uri = builder.build();

        // Send the request and decode the response into (filename : data) pairs
        HttpGet request = new HttpGet(uri);
        try (CloseableHttpResponse response = client.execute(request)) {
            assertEquals(expected_status, response.getStatusLine().getStatusCode());
            if(expected_status==200) {

		// Check that this is a tar file
		Header contentTypeHeader = response.getFirstHeader("Content-Type");
		assertNotNull(contentTypeHeader);
		assertEquals("application/x-tar", contentTypeHeader.getValue());

		// Now read the tar file contents
		TarArchiveInputStream instream = new TarArchiveInputStream(response.getEntity().getContent());
		TarArchiveEntry entry = null;
		data = new HashMap<String, byte[]>();
		while ((entry = instream.getNextEntry()) != null) {
		    if (entry.isFile()) {
			String input_path = entry.getName();
			byte[] input_data = instream.readAllBytes();
			data.put(input_path, input_data);
		    }
		}
	    }
        }
        return data;
    }

    public boolean requestReload() throws Exception {

        // Construct URI with the path
        URIBuilder builder = new URIBuilder(baseURL+"/status");
        builder.addParameter("reload", "1");
        URI uri = builder.build();
        HttpUriRequest request = new HttpGet(uri);

        // Send the request and return true if it worked
        try (CloseableHttpResponse response = client.execute(request)) {
            return (response.getStatusLine().getStatusCode() == 200);
        }
    }
}
