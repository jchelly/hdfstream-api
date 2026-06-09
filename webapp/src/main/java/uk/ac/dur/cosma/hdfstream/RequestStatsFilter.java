package uk.ac.dur.cosma.hdfstream;

import javax.servlet.*;
import java.io.IOException;
import java.util.concurrent.atomic.AtomicLong;

public class RequestStatsFilter implements Filter {

    private final AtomicLong requestCount = new AtomicLong();

    @Override
    public void doFilter(ServletRequest request, ServletResponse response, FilterChain chain)
            throws IOException, ServletException {

        requestCount.incrementAndGet();  // increment request count
        chain.doFilter(request, response);
    }

    public long getRequestCount() {
        return requestCount.get();
    }

    @Override
    public void init(FilterConfig filterConfig) {
        filterConfig.getServletContext().setAttribute("requestStatsFilter", this);
    }
}
