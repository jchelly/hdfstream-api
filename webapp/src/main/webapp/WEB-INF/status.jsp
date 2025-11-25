<!DOCTYPE html>
<html>

<%@ taglib prefix="c" uri="http://java.sun.com/jsp/jstl/core" %>
<%@ taglib prefix="fmt" uri="http://java.sun.com/jsp/jstl/fmt" %>

<head></head>

<body>

  <div class="body-text">
    <h2>Server status</h2>
    <p>
      <c:set var="now" value="<%=new java.util.Date()%>" />
      <fmt:formatDate value="${now}" type="date" />, <fmt:formatDate value="${now}" type="time" />
    </p>
    <p>
      Total configured data: <c:out value="${total_size}"/>
    </p>
    <h3>Process pool</h3>
    <p>
      Number of reader processes: <c:out value="${nr_processes}" />
    </p>
    <ol start="0">
      <c:forEach items = "${cache_info}" var="ci">
        <li>
          <c:if test = "${ci.state == 0}">
            Idle
          </c:if>
          <c:if test = "${ci.state == 1}">
            Busy
          </c:if>
          <c:if test = "${ci.state == 2}">
            Stopped
          </c:if>
          , file cache hits <c:out value="${ci.hits}" />, file cache misses <c:out value="${ci.misses}" />
        </li>
      </c:forEach>
    </ol>
    <h3>Requests currently running (if any)</h3>
    <p>

      <ul>
        <c:forEach var="entry" items="${concurrent_request_count.getCounts()}">
          <li>User <c:out value="${entry.key}"/> has <c:out value="${entry.value}"/> request(s) running</li>
        </c:forEach>
      </ul>

    </p>

  </div>

</body>
</html>
