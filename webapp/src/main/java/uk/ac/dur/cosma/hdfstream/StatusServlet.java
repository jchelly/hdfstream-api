package uk.ac.dur.cosma.hdfstream;

import javax.servlet.Servlet;
import javax.servlet.ServletException;
import javax.servlet.ServletOutputStream;
import javax.servlet.ServletContext;
import javax.servlet.RequestDispatcher;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.IOException;
import uk.ac.dur.cosma.virtual_directory.VirtualDirectory;
import uk.ac.dur.cosma.virtual_directory.VirtualDirectoryException;
import uk.ac.dur.cosma.libhdfstream.*;

public class StatusServlet extends HttpServlet implements Servlet {

    public StatusServlet() {}

    protected void doGet(HttpServletRequest request, HttpServletResponse response)
	throws ServletException, IOException
    {
	doHeadOrGet(request, response, true);
    }

    protected void doHead(HttpServletRequest request, HttpServletResponse response)
	throws ServletException, IOException
    {
	doHeadOrGet(request, response, false);
    }

    // Handle HEAD or GET requests
    // Procedure is the same in either case except that for HEAD we return without writing the response body
    protected void doHeadOrGet(HttpServletRequest request, HttpServletResponse response, boolean is_get)
	throws ServletException, IOException
    {
        ServletContext context = getServletContext();
        String message = null;

        // Check we have a virtual directory config
        ConfigManager config = (ConfigManager) context.getAttribute("config");
        if(config == null) {
            message = "Failed to read virtual directory configuration";
        } else {
            message = "Server started";
        }

        // Check for the reload flag
        String reload = request.getParameter("reload");
        if((config != null) && (reload != null) && reload.equals("1")) {
            try {
                config.reload();
                message = "Server configuration reloaded";
            } catch (VirtualDirectoryException e) {
                message = "Reload failed: "+e.getMessage();
            }
        }

        // Return an internal server error if hdfstream didn't start
	HDFStream hs = (HDFStream) context.getAttribute("hdfstream");
        if(hs==null) {
            response.sendError(HttpServletResponse.SC_INTERNAL_SERVER_ERROR, "Hdfstream library initialization failed");
            return;
        }

        // Determine number of processes
        Integer nr_processes = (Integer) context.getAttribute("nr_processes");
        request.setAttribute("nr_processes", nr_processes);

        // Get cache usage stats
        int n = Integer.valueOf(nr_processes);
        HDFStreamCacheInfo[] cache_info = new HDFStreamCacheInfo[n];
        for(int i=0; i<n; i+=1) {
            cache_info[i] = hs.getCacheInfo(i);
        }
        request.setAttribute("cache_info", cache_info);

        // Get total data size as a string
        VirtualDirectory root = config.getRoot();
        long size = root.getTotalSize((in) -> true);
        String total_size = VirtualDirectory.formatSize(size);
        request.setAttribute("total_size", total_size);

        // Store any status message
        request.setAttribute("status_message", message);

        // Forward this request to the directory status page
        RequestDispatcher dispatcher = request.getRequestDispatcher("/WEB-INF/status.jsp");
        dispatcher.forward(request, response);
    }
}
