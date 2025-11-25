<!DOCTYPE html>
<html>

  <%@ taglib prefix="c" uri="http://java.sun.com/jsp/jstl/core" %>

  <head></head>

  <body>

    <div class="body-text">

      <h2>Request failed</h2>

      <p>
        Status <c:out value="${requestScope['javax.servlet.error.status_code']}"/>
      </p>
      <p>
        <c:out value="${requestScope['javax.servlet.error.message']}"/>
      </p>
      
    </div>
    
  </body>
</html>
