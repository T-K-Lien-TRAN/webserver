FROM nginx:alpine

# Copy custom nginx configuration
COPY eval.conf /etc/nginx/eval.conf

# Copy website files (create a simple index.html if none exists)
COPY ./www /usr/share/nginx/www

# Expose port 80
EXPOSE 80

# Start nginx
CMD ["nginx", "-g", "daemon off;"]